using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UnrealBuildTool;
using BuildGraph;
using System.Reflection;
using System.Collections;
using System.IO;

namespace AutomationTool
{
	/// <summary>
	/// Tool to execute build processes for UE4 projects, which can be run locally or in parallel across a build farm (assuming synchronization and resource allocation implemented by a separate system).
	///
	/// Build graphs are declared using an XML script using syntax similar to MSBuild, ANT or NAnt, and consist of the following components:
	///
	/// - Tasks:        Building blocks which can be executed as part of the build process. Many predefined tasks are provided ('Cook', 'Compile', 'Copy', 'Stage', 'Log', 'PakFile', etc...), and additional tasks may be 
	///                 added be declaring classes derived from AutomationTool.CustomTask in other UAT modules. 
	/// - Nodes:        A named sequence of tasks which are executed in order to produce outputs. Nodes may have dependencies on other nodes for their outputs before they can be executed. Declared with the 'Node' element.
	/// - Agent Groups: A set of nodes nodes which is executed on the same machine if running as part of a build system. Has no effect when building locally. Declared with the 'Group' element.
	/// - Triggers:     Container for groups which should only be executed when explicitly triggered (using the -Trigger=... or -SkipTriggers command line argument). Declared with the 'Trigger' element.
	/// - Notifiers:    Specifies email recipients for failures in one or more nodes, whether they should receive notifications on warnings, and so on.
	/// 
	/// Properties can be passed in to a script on the command line, or set procedurally with the &ltProperty Name="Foo" Value="Bar"/&gt; syntax. Properties referenced with the $(Property Name) notation are valid within 
	/// all strings, and will be expanded as macros when the script is read. If a property name is not set explicitly, it defaults to the contents of an environment variable with the same name. 
	/// Local properties, which only affect the scope of the containing XML element (node, group, etc...) are declared with the &lt;Local Name="Foo" Value="Bar"/&gt; element, and will override a similarly named global 
	/// property for the local property's scope.
	///
	/// Any elements can be conditionally defined via the "If" attribute, which follows a syntax similar to MSBuild. Literals in conditions may be quoted with single (') or double (") quotes, or an unquoted sequence of 
	/// letters, digits and underscore characters. All literals are considered identical regardless of how they are declared, and are considered case-insensitive for comparisons (so true equals 'True', equals "TRUE"). 
	/// Available operators are "==", "!=", "And", "Or", "!", "(...)", "Exists(...)" and "HasTrailingSlash(...)". A full grammar is written up in Condition.cs.
	/// 
	/// File manipulation is done using wildcards and tags. Any attribute that accepts a list of files may consist of: a Perforce-style wildcard (matching any number of "...", "*" and "?" patterns in any location), a 
	/// full path name, or a reference to a tagged collection of files, denoted by prefixing with a '#' character. Files may be added to a tag set using the &lt;Tag&gt; Task, which also allows performing set union/difference 
	/// style operations. Each node can declare multiple outputs in the form of a list of named tags, which other nodes can then depend on.
	/// 
	/// Build graphs may be executed in parallel as part build system. To do so, the initial graph configuration is generated by running with the -Export=... argument (producing a JSON file listing the nodes 
	/// and dependencies to execute). Each participating agent should be synced to the same changelist, and UAT should be re-run with the appropriate -Node=... argument. Outputs from different nodes are transferred between 
	/// agents via shared storage, typically a network share, the path to which can be specified on the command line using the -SharedStorageDir=... argument. Note that the allocation of machines, and coordination between 
	/// them, is assumed to be managed by an external system based on the contents of the script generated by -Export=....
	/// 
	/// A schema for the known set of tasks can be generated by running UAT with the -Schema=... option. Generating a schema and referencing it from a BuildGraph script allows Visual Studio to validate and auto-complete 
	/// elements as you type.
	/// </summary>
	[Help("Tool for creating extensible build processes in UE4 which can be run locally or in parallel across a build farm.")]
	[Help("Script=<FileName>", "Path to the script describing the graph")]
	[Help("Target=<Name>", "Name of the node or output tag to be built")]
	[Help("Schema=<FileName>", "Generate a schema describing valid script documents, including all the known tasks")]
	[Help("Set:<Property>=<Value>", "Sets a named property to the given value")]
	[Help("Clean", "Cleans all cached state of completed build nodes before running")]
	[Help("CleanNode=<Name>[+<Name>...]", "Cleans just the given nodes before running")]
	[Help("ListOnly", "Shows the contents of the preprocessed graph, but does not execute it")]
	[Help("ShowDeps", "Show node dependencies in the graph output")]
	[Help("ShowNotifications", "Show notifications that will be sent for each node in the output")]
	[Help("Trigger=<Name>[+<Name>...]", "Activates the given triggers, including all the nodes behind them in the graph")]
	[Help("SkipTriggers", "Activate all triggers")]
	[Help("Export=<FileName>", "Exports a JSON file containing the preprocessed build graph, for use as part of a build system")]
	[Help("PublicTasksOnly", "Only include built-in tasks in the schema, excluding any other UAT modules")]
	[Help("SharedStorageDir=<DirName>", "Sets the directory to use to transfer build products between agents in a build farm")]
	[Help("SingleNode=<Name>", "Run only the given node. Intended for use on a build system after running with -Export.")]
	[Help("WriteToSharedStorage", "Allow writing to shared storage. If not set, but -SharedStorageDir is specified, build products will read but not written")]
	public class Build : BuildCommand
	{
		/// <summary>
		/// Main entry point for the BuildGraph command
		/// </summary>
		public override ExitCode Execute()
		{
			// Parse the command line parameters
			string ScriptFileName = ParseParamValue("Script", null);
			if(ScriptFileName == null)
			{
				LogError("Missing -Script= parameter for BuildGraph");
				return ExitCode.Error_Unknown;
			}

			string TargetNames = ParseParamValue("Target", null);
			if(TargetNames == null)
			{
				LogError("Missing -Target= parameter for BuildGraph");
				return ExitCode.Error_Unknown;
			}

			string SchemaFileName = ParseParamValue("Schema", null);
			string ExportFileName = ParseParamValue("Export", null);

			string SharedStorageDir = ParseParamValue("SharedStorageDir", null);
			string SingleNodeName = ParseParamValue("SingleNode", null);
			string[] TriggerNames = ParseParamValue("Trigger", "").Split(new char[]{ '+' }, StringSplitOptions.RemoveEmptyEntries).ToArray();
			bool bSkipTriggers = ParseParam("SkipTriggers");
			bool bClean = ParseParam("Clean");
			bool bListOnly = ParseParam("ListOnly");
			bool bWriteToSharedStorage = ParseParam("WriteToSharedStorage") || CommandUtils.IsBuildMachine;
			bool bPublicTasksOnly = ParseParam("PublicTasksOnly");

			GraphPrintOptions PrintOptions = 0;
			if(ParseParam("ShowDeps"))
			{
				PrintOptions |= GraphPrintOptions.ShowDependencies;
			}
			if(ParseParam("ShowNotifications"))
			{
				PrintOptions |= GraphPrintOptions.ShowNotifications;
			}

			// Parse any specific nodes to clean
			List<string> CleanNodes = new List<string>();
			foreach(string NodeList in ParseParamValues("CleanNode"))
			{
				foreach(string NodeName in NodeList.Split('+'))
				{
					CleanNodes.Add(NodeName);
				}
			}

			// Read any environment variables
			Dictionary<string, string> DefaultProperties = new Dictionary<string,string>(StringComparer.InvariantCultureIgnoreCase);
			foreach(DictionaryEntry Entry in Environment.GetEnvironmentVariables())
			{
				DefaultProperties[Entry.Key.ToString()] = Entry.Value.ToString();
			}

			// Add any additional custom parameters from the command line (of the form -Set:X=Y)
			foreach(string Param in Params)
			{
				const string Prefix = "-Set:";
				if(Param.StartsWith(Prefix, StringComparison.InvariantCultureIgnoreCase))
				{
					int EqualsIdx = Param.IndexOf('=');
					if(EqualsIdx >= 0)
					{
						DefaultProperties[Param.Substring(Prefix.Length, EqualsIdx - Prefix.Length)] = Param.Substring(EqualsIdx + 1);
					}
				}
			}

			// Set up the standard properties which build scripts might need
			DefaultProperties["Branch"] = P4Enabled? P4Env.BuildRootP4 : "Unknown";
			DefaultProperties["EscapedBranch"] = P4Enabled? P4Env.BuildRootEscaped : "Unknown";
			DefaultProperties["Change"] = P4Enabled? P4Env.Changelist.ToString() : "0";
			DefaultProperties["RootDir"] = new DirectoryReference(CommandUtils.CmdEnv.LocalRoot).FullName;
			DefaultProperties["IsBuildMachine"] = IsBuildMachine? "true" : "false";
			DefaultProperties["HostPlatform"] = HostPlatform.Current.HostEditorPlatform.ToString();

			// Find all the tasks from the loaded assemblies
			Dictionary<string, ScriptTask> NameToTask = new Dictionary<string,ScriptTask>();
			if(!FindAvailableTasks(NameToTask, bPublicTasksOnly))
			{
				return ExitCode.Error_Unknown;
			}

			// Create a schema for the given tasks
			ScriptSchema Schema = new ScriptSchema(NameToTask);
			if(SchemaFileName != null)
			{
				Schema.Export(new FileReference(SchemaFileName));
			}

			// Read the script from disk
			Graph Graph;
			if(!ScriptReader.TryRead(new FileReference(ScriptFileName), DefaultProperties, Schema, out Graph))
			{
				return ExitCode.Error_Unknown;
			}

			// Create the temp storage handler
			DirectoryReference RootDir = new DirectoryReference(CommandUtils.CmdEnv.LocalRoot);
			TempStorage Storage = new TempStorage(RootDir, DirectoryReference.Combine(RootDir, "Engine", "Saved", "BuildGraph"), (SharedStorageDir == null)? null : new DirectoryReference(SharedStorageDir), bWriteToSharedStorage);
			if(bClean)
			{
				Storage.CleanLocal();
			}
			foreach(string CleanNode in CleanNodes)
			{
				Storage.CleanLocalNode(CleanNode);
			}

			// Convert the supplied target references into nodes 
			HashSet<Node> TargetNodes = new HashSet<Node>();
			foreach(string TargetName in TargetNames.Split(new char[]{ '+' }, StringSplitOptions.RemoveEmptyEntries).Select(x => x.Trim()))
			{
				Node[] Nodes;
				if(!Graph.TryResolveReference(TargetName, out Nodes))
				{
					LogError("Target '{0}' is not in graph", TargetName);
					return ExitCode.Error_Unknown;
				}
				TargetNodes.UnionWith(Nodes);
			}

			// Cull the graph to include only those nodes
			Graph.Select(TargetNodes);

			// Find the triggers which are explicitly activated, and all of its upstream triggers.
			HashSet<ManualTrigger> Triggers = new HashSet<ManualTrigger>();
			foreach(string TriggerName in TriggerNames)
			{
				ManualTrigger Trigger;
				if(!Graph.NameToTrigger.TryGetValue(TriggerName, out Trigger))
				{
					LogError("Couldn't find trigger '{0}'", TriggerName);
					return ExitCode.Error_Unknown;
				}
				while(Trigger != null)
				{
					Triggers.Add(Trigger);
					Trigger = Trigger.Parent;
				}
			}
			if(bSkipTriggers)
			{
				Triggers.UnionWith(Graph.NameToTrigger.Values);
			}

			// If we're just building a single node, find it 
			Node SingleNode = null;
			if(SingleNodeName != null && !Graph.NameToNode.TryGetValue(SingleNodeName, out SingleNode))
			{
				LogError("Node '{0}' is not in the trimmed graph", SingleNodeName);
				return ExitCode.Error_Unknown;
			}

			// Execute the command
			if(bListOnly)
			{ 
				HashSet<Node> CompletedNodes = FindCompletedNodes(Graph, Storage);
				Graph.Print(CompletedNodes, PrintOptions);
			}
			else if(ExportFileName != null)
			{
				HashSet<Node> CompletedNodes = FindCompletedNodes(Graph, Storage);
				Graph.Print(CompletedNodes, PrintOptions);
				Graph.Export(new FileReference(ExportFileName), Triggers, CompletedNodes);
			}
			else if(SingleNode != null)
			{
				if(!BuildSingleNode(new JobContext(this), Graph, SingleNode, Storage))
				{
					return ExitCode.Error_Unknown;
				}
			}
			else
			{
				if(!BuildAllNodes(new JobContext(this), Graph, Storage))
				{
					return ExitCode.Error_Unknown;
				}
			}
			return ExitCode.Success;
		}

		/// <summary>
		/// Find all the tasks which are available from the loaded assemblies
		/// </summary>
		/// <param name="TaskNameToReflectionInfo">Mapping from task name to information about how to serialize it</param>
		/// <param name="bPublicTasksOnly">Whether to include just public tasks, or all the tasks in any loaded assemblies</param>
		static bool FindAvailableTasks(Dictionary<string, ScriptTask> NameToTask, bool bPublicTasksOnly)
		{
			Assembly[] LoadedAssemblies = AppDomain.CurrentDomain.GetAssemblies();
			if(bPublicTasksOnly)
			{
				LoadedAssemblies = LoadedAssemblies.Where(x => IsPublicAssembly(new FileReference(x.Location))).ToArray();
			}
			foreach (Assembly LoadedAssembly in LoadedAssemblies)
			{
				Type[] Types = LoadedAssembly.GetTypes();
				foreach(Type Type in Types)
				{
					foreach(TaskElementAttribute ElementAttribute in Type.GetCustomAttributes<TaskElementAttribute>())
					{
						if(!Type.IsSubclassOf(typeof(CustomTask)))
						{
							CommandUtils.LogError("Class '{0}' has TaskElementAttribute, but is not derived from 'Task'", Type.Name);
							return false;
						}
						if(NameToTask.ContainsKey(ElementAttribute.Name))
						{
							CommandUtils.LogError("Found multiple handlers for task elements called '{0}'", ElementAttribute.Name);
							return false;
						}
						NameToTask.Add(ElementAttribute.Name, new ScriptTask(ElementAttribute.Name, Type, ElementAttribute.ParametersType));
					}
				}
			}
			return true;
		}

		/// <summary>
		/// Checks whether the given assembly is a publically distributed engine assembly.
		/// </summary>
		/// <param name="File">Assembly location</param>
		/// <returns>True if the assembly is distributed publically</returns>
		static bool IsPublicAssembly(FileReference File)
		{
			DirectoryReference EngineDirectory = UnrealBuildTool.UnrealBuildTool.EngineDirectory;
			if(File.IsUnderDirectory(EngineDirectory))
			{
				string[] PathFragments = File.MakeRelativeTo(EngineDirectory).Split(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
				if(PathFragments.All(x => !x.Equals("NotForLicensees", StringComparison.InvariantCultureIgnoreCase) && !x.Equals("NoRedist", StringComparison.InvariantCultureIgnoreCase)))
				{
					return true;
				}
			}
			return false;
		}

		/// <summary>
		/// Find all the nodes in the graph which are already completed
		/// </summary>
		/// <param name="Graph">The graph instance</param>
		/// <param name="Storage">The temp storage backend which stores the shared state</param>
		HashSet<Node> FindCompletedNodes(Graph Graph, TempStorage Storage)
		{
			HashSet<Node> CompletedNodes = new HashSet<Node>();
			foreach(Node Node in Graph.Groups.SelectMany(x => x.Nodes))
			{
				if(Storage.IsComplete(Node.Name))
				{
					CompletedNodes.Add(Node);
				}
			}
			return CompletedNodes;
		}

		/// <summary>
		/// Builds all the nodes in the graph
		/// </summary>
		/// <param name="Job">Information about the current job</param>
		/// <param name="Graph">The graph instance</param>
		/// <returns>True if everything built successfully</returns>
		bool BuildAllNodes(JobContext Job, Graph Graph, TempStorage Storage)
		{
			// Build a flat list of nodes to execute, in order
			Node[] NodesToExecute = Graph.Groups.SelectMany(x => x.Nodes).ToArray();

			// Check the integrity of any local nodes that have been completed. It's common to run formal builds locally between regular development builds, so we may have 
			// stale local state. Rather than failing later, detect and clean them up now.
			HashSet<Node> CleanedNodes = new HashSet<Node>();
			foreach(Node NodeToExecute in NodesToExecute)
			{
				if(NodeToExecute.InputDependencies.Any(x => CleanedNodes.Contains(x)) || !Storage.CheckLocalIntegrity(NodeToExecute.Name, NodeToExecute.OutputNames))
				{
					Storage.CleanLocalNode(NodeToExecute.Name);
					CleanedNodes.Add(NodeToExecute);
				}
			}

			// Execute them in order
			int NodeIdx = 0;
			foreach(Node NodeToExecute in NodesToExecute)
			{
				Log("****** [{0}/{1}] {2} ******", ++NodeIdx, NodesToExecute.Length, NodeToExecute.Name);
				if(!Storage.IsComplete(NodeToExecute.Name))
				{
					Log("");
					if(!BuildSingleNode(Job, Graph, NodeToExecute, Storage))
					{
						return false;
					} 
					Log("");
				}
			}
			return true;
		}

		/// <summary>
		/// Build a single node
		/// </summary>
		/// <param name="Job">Information about the current job</param>
		/// <param name="Graph">The graph to which the node belongs. Used to determine which outputs need to be transferred to temp storage.</param>
		/// <param name="Node">The node to build</param>
		/// <returns>True if the node built successfully, false otherwise.</returns>
		bool BuildSingleNode(JobContext Job, Graph Graph, Node Node, TempStorage Storage)
		{
			// Create a mapping from tag name to the files it contains, and seed it with invalid entries for everything in the graph
			Dictionary<string, HashSet<FileReference>> TagNameToFileSet = new Dictionary<string,HashSet<FileReference>>();
			foreach(string OutputName in Graph.Groups.SelectMany(x => x.Nodes).SelectMany(x => x.OutputNames))
			{
				TagNameToFileSet[OutputName] = null;
			}

			// Fetch all the input dependencies for this node, and fill in the tag names with those files
			DirectoryReference RootDir = new DirectoryReference(CommandUtils.CmdEnv.LocalRoot);
			foreach(string InputName in Node.InputNames)
			{
				Node InputNode = Graph.OutputNameToNode[InputName];
				TempStorageManifest Manifest = Storage.Retreive(InputNode.Name, InputName);
				TagNameToFileSet[InputName] = new HashSet<FileReference>(Manifest.Files.Select(x => x.ToFileReference(RootDir)));
			}

			// Add placeholder outputs for the current node
			foreach(string OutputName in Node.OutputNames)
			{
				TagNameToFileSet[OutputName] = new HashSet<FileReference>();
			}

			// Execute the node
			if(!Node.Build(Job, TagNameToFileSet))
			{
				return false;
			}

			// Determine all the outputs which are required to be copied to temp storage (because they're referenced by nodes in another agent group)
			HashSet<string> ReferencedOutputs = new HashSet<string>();
			foreach(AgentGroup Group in Graph.Groups)
			{
				bool bSameGroup = Group.Nodes.Contains(Node);
				foreach(Node OtherNode in Group.Nodes)
				{
					if(!bSameGroup || Node.ControllingTrigger != OtherNode.ControllingTrigger)
					{
						ReferencedOutputs.UnionWith(OtherNode.InputNames);
					}
				}
			}

			// Publish all the outputs
			foreach(string OutputName in Node.OutputNames)
			{
				Storage.Archive(Node.Name, OutputName, TagNameToFileSet[OutputName].ToArray(), ReferencedOutputs.Contains(OutputName));
			}

			// Mark the node as succeeded
			Storage.MarkAsComplete(Node.Name);
			return true;
		}

		/// <summary>
		/// Build a list of output files to their output name
		/// </summary>
		/// <param name="Node"></param>
		/// <param name="TagNameToFileSet"></param>
		/// <param name="FileToOutputName"></param>
		/// <returns></returns>
		static bool FindFileToOutputNameMapping(Node Node, Dictionary<string, HashSet<FileReference>> TagNameToFileSet, Dictionary<FileReference, string> FileToOutputName)
		{
			bool bResult = true;
			foreach(string OutputName in Node.OutputNames)
			{
				HashSet<FileReference> FileSet;
				if(TagNameToFileSet.TryGetValue(OutputName, out FileSet))
				{
					foreach(FileReference File in FileSet)
					{
						string ExistingOutputName;
						if(FileToOutputName.TryGetValue(File, out ExistingOutputName))
						{
							CommandUtils.LogError("Build product is added to multiple outputs; {0} added to {1} and {2}", File.MakeRelativeTo(new DirectoryReference(CommandUtils.CmdEnv.LocalRoot)), ExistingOutputName, OutputName);
							bResult = false;
							continue;
						}
						FileToOutputName.Add(File, OutputName);
					}
				}
			}
			return bResult;
		}
	}
}
