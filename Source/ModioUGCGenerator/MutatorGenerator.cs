using EpicGames.Core;
using EpicGames.UHT.Parsers;
using EpicGames.UHT.Tables;
using EpicGames.UHT.Tokenizer;
using EpicGames.UHT.Types;
using EpicGames.UHT.Utils;
using System;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Reflection.Metadata;
using System.Reflection.PortableExecutable;
using System.Text.RegularExpressions;
using System.Xml.Linq;
using static System.Formats.Asn1.AsnWriter;

namespace ModioUGCGenerator.MutatorGenerator
{
    //Custom Script Struct class to facilitate generating headers for structs with deferred properties
    public class UhtModScriptStruct : UhtScriptStruct
    {
        public bool bNeedsResolution { get; set; } = false;
        public UHTManifest.Module? ModModule { get; set; } = null;

#if UE_5_7_OR_LATER
        public UhtModScriptStruct(UhtHeaderFile HeaderFile, UhtNamespace NamespaceObj, UhtType Outer, int LineNumber)
        : base(HeaderFile, NamespaceObj, Outer, LineNumber)
        {
        }
#elif UE_5_5_OR_LATER
        public UhtModScriptStruct(UhtHeaderFile HeaderFile, UhtType outer, int lineNumber)
        : base(HeaderFile, outer, lineNumber)
        {
        }
#else
        public UhtModScriptStruct(UhtType outer, int lineNumber)
        : base(outer, lineNumber)
        {
        }
#endif

        protected override bool ResolveSelf(UhtResolvePhase resolvePhase)
        {
            bool Result = base.ResolveSelf(resolvePhase);

            //If this struct header generation has been deferred write it out now
            if(resolvePhase == UhtResolvePhase.Final && bNeedsResolution)
            {
                UhtMutatorGenerator.WriteStructInclude(ModModule!, this);
            }

            return Result;
        }
    }

    [UnrealHeaderTool]
    public static class UhtMutatorGenerator
    {
        [UhtKeyword(Extends = UhtTableNames.Class)]
        [SuppressMessage("CodeQuality", "IDE0051:Remove unused private members", Justification = "Attribute accessed method")]
        [SuppressMessage("Style", "IDE0060:Remove unused parameter", Justification = "Attribute accessed method")]
        private static UhtParseResult DEFINE_MUTATORKeyword(UhtParsingScope topScope, UhtParsingScope actionScope, ref UhtToken token)
        {
            return ParseMutatorDefine(topScope, token);
        }

        [UhtKeyword(Extends = UhtTableNames.Class)]
        [SuppressMessage("CodeQuality", "IDE0051:Remove unused private members", Justification = "Attribute accessed method")]
        [SuppressMessage("Style", "IDE0060:Remove unused parameter", Justification = "Attribute accessed method")]
        private static UhtParseResult DEFINE_MUTATOR_RETURNKeyword(UhtParsingScope topScope, UhtParsingScope actionScope, ref UhtToken token)
        {
            return ParseMutatorDefine(topScope, token, true);
        }

        /**
         * Parse the DEFINE_MUTATOR macros. 
         * Generates:
         *          - Blueprint VM thunk
         *          - Struct declaration for function parameters
         */
        private static UhtParseResult ParseMutatorDefine(UhtParsingScope topScope, UhtToken token, bool bHasReturnType = false)
        {
            //Create new function that generates the blueprint callable thunk
#if UE_5_7_OR_LATER
            UhtFunction function = new(topScope.HeaderFile, topScope.HeaderParser.GetNamespace(), topScope.ScopeType, token.InputLine);
#elif UE_5_5_OR_LATER
            UhtFunction function = new(topScope.HeaderFile, topScope.ScopeType, token.InputLine);
#else
            UhtFunction function = new(topScope.ScopeType, token.InputLine);
#endif

            //Set up function specifiers so it can be called in Blueprints
            function.MacroLineNumber = topScope.TokenReader.InputLine;
            function.FunctionType = UhtFunctionType.Function;
            function.FunctionFlags |= EFunctionFlags.Native;
            function.FunctionFlags |= EFunctionFlags.BlueprintCallable;

            //Make sure the function access is the same as where the macro is used
            switch (topScope.AccessSpecifier)
            {
                case UhtAccessSpecifier.Public:
                    function.FunctionFlags |= EFunctionFlags.Public;
                    break;

                case UhtAccessSpecifier.Protected:
                    function.FunctionFlags |= EFunctionFlags.Protected;
                    break;

                case UhtAccessSpecifier.Private:
                    function.FunctionFlags |= EFunctionFlags.Private | EFunctionFlags.Final;
                    break;
            }

            //Parse the macro parameters
            List<UhtToken> tokens = new List<UhtToken>();
            topScope.TokenReader.Require('(');

            //Parse name
            UhtToken Name = topScope.TokenReader.GetToken();

            //Create new struct to be used as a parameter for the new event
            UhtType StructOuter = topScope.ParentScope == null ? topScope.ScopeType : topScope.ParentScope.ScopeType;
#if UE_5_7_OR_LATER
            UhtModScriptStruct ParamStruct = new(topScope.HeaderFile, topScope.HeaderParser.GetNamespace(), StructOuter, token.InputLine);
#elif UE_5_5_OR_LATER
            UhtModScriptStruct ParamStruct = new(topScope.HeaderFile, StructOuter, token.InputLine);
#else
            UhtModScriptStruct ParamStruct = new(StructOuter, token.InputLine);
#endif
            ParamStruct.ScriptStructFlags |= EStructFlags.Native;
            ParamStruct.ScriptStructFlags |= EStructFlags.NoExport;
            ParamStruct.EngineName = Name.Value.ToString() + "_Params";
            ParamStruct.SourceName = "F" + ParamStruct.EngineName;
            ParamStruct.MacroDeclaredLineNumber = token.InputLine;
            ParamStruct.MetaData.Add(UhtNames.BlueprintType, true);

            //If there are other arguments convert them into parameters for the event
            if (topScope.TokenReader.PeekToken().Value.ToString().Equals(","))
            {
                //Parse param member list, adding them to the param struct
                List<UhtProperty> Members = new List<UhtProperty>();
                ParseMacroParameters(topScope, ParamStruct, ref Members);
            }

            //Add the new param struct to the header to be generated
#if UE_5_5_OR_LATER
            topScope.HeaderFile.AddChild(ParamStruct);
#else
            topScope.CurrentClass.HeaderFile.AddChild(ParamStruct);
#endif

            //Add a parameter to the new param struct type to the parameter list of the function
            UhtPropertySettings StructParamSettings = new();
            StructParamSettings.PropertyCategory = UhtPropertyCategory.RegularParameter;
            StructParamSettings.SourceName = "Params";
            StructParamSettings.EngineName = "Params";
            StructParamSettings.Outer = function;
            UhtStructProperty StructParamProp = new(StructParamSettings, ParamStruct);
            StructParamProp.PropertyFlags |= EPropertyFlags.Parm;
            StructParamProp.PropertyCaps |= UhtPropertyCaps.IsParameterSupportedByBlueprint;
            function.AddChild(StructParamProp);

            //If this function needs a return type add it here
            if (bHasReturnType)
            {
                AddReturnType(function, ParamStruct);
                ParamStruct.bNeedsResolution = true;
            }

            //Write the struct to a header to be added to the consolidated include
            GenerateParamsStructHeader(topScope, ParamStruct);


            //Create function that generates the native declaration 
#if UE_5_7_OR_LATER
            UhtFunction MutatorFunction = new(topScope.HeaderFile, topScope.HeaderParser.GetNamespace(), topScope.ScopeType, token.InputLine);
#elif UE_5_5_OR_LATER
            UhtFunction MutatorFunction = new(topScope.HeaderFile, topScope.ScopeType, token.InputLine);
#else
            UhtFunction MutatorFunction = new(topScope.ScopeType, token.InputLine);
#endif
            MutatorFunction.SourceName = Name.Value.ToString();
            MutatorFunction.EngineName = Name.Value.ToString();

            //Add the implementation function to be generated
            MutatorFunction.Outer?.AddChild(MutatorFunction);

            //Fill out details for the thunk based on the implementation function
            function.SourceName = "Mutator_" + MutatorFunction.SourceName;
            function.UnMarshalAndCallName = "execMutator_" + MutatorFunction.EngineName;
            function.CppImplName = MutatorFunction.EngineName;

            //Set up some meta data to make the generated function match out standards
            function.MetaData.Add(UhtNames.Category, "mod.io|Mutators|Events");
            function.MetaData.Add(UhtNames.DisplayName, MutatorFunction.SourceName);

            //Add the thunk to get generated
            function.Outer?.AddChild(function);

            return UhtParseResult.Handled;
        }

        /**
         * Parse the parameter arguments into members of the generated parameters struct
         * @param Scope the scope the is currently being parsed
         * @param MemberOuter the struct we are adding members to
         * @param Members a list of properties that we added to the struct
         */
        private static void ParseMacroParameters(UhtParsingScope Scope, UhtModScriptStruct MemberOuter, ref List<UhtProperty> Members)
        {
            //Parse each of the remaining arguments into lists of tokens for later
            IUhtTokenReader TokenReader = Scope.TokenReader;
            List<List<UhtToken>> MemberTokens = new List<List<UhtToken>>();
            TokenReader.RequireList(',', ')', ',', false, (IEnumerable<UhtToken> tokens) => 
            {
                MemberTokens.Add(tokens.ToList());
            });

            //Parameter count error checking
            if (MemberTokens.Count % 2 != 0)
            {
                Scope.Session.LogError($"Parameters must come in pairs [type + name]!");
            }

            //Process each [Type, Name] pair of arguments found
            foreach (var ParameterInfo in MemberTokens.Chunk(2))
            {
                //Type info: ParameterInfo[0]
                //Name: ParameterInfo[1]

                UhtToken NameToken = ParameterInfo.Last().First();

                UhtPropertySettings MemberPropertySettings = new UhtPropertySettings();
                MemberPropertySettings.PropertyCategory = UhtPropertyCategory.Member;
                MemberPropertySettings.SourceName = MakeValidParameterName(NameToken);
                MemberPropertySettings.EngineName = MemberPropertySettings.SourceName;
                MemberPropertySettings.LineNumber = NameToken.InputLine;
                MemberPropertySettings.Outer = MemberOuter;

                //TODO: Handle qualifiers

                //The property type might not be available yet. If it isn't then we want to defer the generation on the struct header file to a later parsing phase
                bool bNeedsResolution = false;
                UhtProperty? MemberProperty = MakePropertyFromTokens(Scope, ParameterInfo.First(), MemberPropertySettings, ref bNeedsResolution);
                if(MemberProperty != null)
                {
                    Members.Add(MemberProperty);
                    MemberOuter.AddChild(MemberProperty);
                    MemberOuter.bNeedsResolution = bNeedsResolution;
                }
                else
                {
                    Scope.Session.LogError($"Mutator events do not support type {DetermineParsedParameterTypeToken(ParameterInfo.First()).Value.ToString()}");
                }
            }
        }

        /**
         * Add a return type to a function
         * @param FunctionOuter the function to add a return type to
         * @param ReturnType the type to add for the return type
         */
        private static void AddReturnType(UhtFunction FunctionOuter, UhtModScriptStruct ReturnType)
		{
			//Set up the property settings for the return type
			UhtPropertySettings ReturnPropertySettings = new UhtPropertySettings();
			ReturnPropertySettings.PropertyCategory = UhtPropertyCategory.Member;
			ReturnPropertySettings.PropertyFlags |= EPropertyFlags.Parm | EPropertyFlags.OutParm | EPropertyFlags.ReturnParm | EPropertyFlags.BlueprintVisible;
			ReturnPropertySettings.SourceName = "ReturnValue";
			ReturnPropertySettings.EngineName = ReturnPropertySettings.SourceName;
			ReturnPropertySettings.LineNumber = ReturnType.MacroDeclaredLineNumber;
			ReturnPropertySettings.Outer = FunctionOuter;
			ReturnPropertySettings.MetaData.Add(UhtNames.Category, "Mutator Events");

            //Create the return type property
			UhtStructProperty StructParamProp = new(ReturnPropertySettings, ReturnType);
			StructParamProp.PropertyFlags |= EPropertyFlags.Parm;
			StructParamProp.PropertyCaps |= UhtPropertyCaps.IsParameterSupportedByBlueprint;

            //Add the return property to the function
			FunctionOuter.AddChild(StructParamProp);
		}

        /**
         * Generate a new property from a list of tokens
         * @param Scope the current scope being parsed
         * @params Tokens the tokens we want to use to generate the property from
         * @params PropertySettings the settings to use for the new property
         * @params bNeedsResolution whether the property needs to defer its header generation
         */
        private static UhtProperty? MakePropertyFromTokens(UhtParsingScope Scope, List<UhtToken> Tokens, UhtPropertySettings PropertySettings, ref bool bNeedsResolution)
        {
            UhtToken Matched = DetermineParsedParameterTypeToken(Tokens);
            UhtToken Copy = Matched;

#if UE_5_5_OR_LATER
            var Header = Scope.HeaderFile;
#else
            var Header = Scope.CurrentClass.HeaderFile;
#endif

            //More complicated declarations (such as templates) require additional parsing of the tokens so we reconstruct the relevant section of it using a replay token reader
            //TODO: parsing templates with more than one type, ie TMap, will need some additional handling due to having an internal ',' in it's declaration
#if UE_5_6_OR_LATER
            //We need to add a terminating symbol to the definition
            UhtToken DeclarationTerminationToken = new UhtToken(UhtTokenType.Symbol);
            DeclarationTerminationToken.Value = ";";
            Tokens.Add(DeclarationTerminationToken);
            IUhtTokenReader ReplayReader = new UhtTokenReplayReader(Header, Header.Data.Memory, new ReadOnlyMemory<UhtToken>(Tokens.ToArray()), UhtTokenType.EndOfDeclaration);
#else
            IUhtTokenReader ReplayReader = UhtTokenReplayReader.GetThreadInstance(Header, Header.Data.Memory, new ReadOnlyMemory<UhtToken>(Tokens.ToArray()), UhtTokenType.EndOfDeclaration);
#endif

            //Lookup the property type by name from the registered list of valid types
            if (Scope.Session.TryGetPropertyType(Matched.Value, out UhtPropertyType PropertyType))
            {
#if UE_5_6_OR_LATER
                //Create a property using its registered factory function
                UhtPropertyResolveArgs Args = new(UhtPropertyResolvePhase.Parsing, PropertySettings, ReplayReader);
                UhtProperty? OutProperty = PropertyType.Delegate(Args);
#else
                //Create a property using its registered factory function
                UhtProperty? OutProperty = PropertyType.Delegate(UhtPropertyResolvePhase.Parsing, PropertySettings, ReplayReader, Matched);
#endif
                if (OutProperty != null)
                {
                    OutProperty.PropertyFlags |= EPropertyFlags.BlueprintVisible;
                    OutProperty.MetaData.Add(UhtNames.Category, "Mutator Events");
                    return OutProperty;
                }
                else
                {
                    Scope.Session.LogError($"Mutator events do not support type {Copy.Value.ToString()}");
                }
            }
            else
            {
                //If we can't find the type, add a pre resolve property in it's place so we can figure it out later

#if UE_5_6_OR_LATER
    			UhtPropertySettings PreResolvePropertySettings = new();
    			PreResolvePropertySettings.Copy(PropertySettings);
    			PreResolvePropertySettings.TypeTokens = new(UhtTypeTokens.Gather(ReplayReader), 0);

    			// Set on the settings object so they survive resolution
    			PreResolvePropertySettings.PropertyFlags |= EPropertyFlags.BlueprintVisible;
    			PreResolvePropertySettings.MetaData.Add(UhtNames.Category, "Mutator Events");

    			UhtPreResolveProperty PreResovleProperty = new UhtPreResolveProperty(PreResolvePropertySettings);
#else
    			PropertySettings.PropertyFlags |= EPropertyFlags.BlueprintVisible;
    			PropertySettings.MetaData.Add(UhtNames.Category, "Mutator Events");

    			UhtPreResolveProperty PreResovleProperty = new UhtPreResolveProperty(PropertySettings, new ReadOnlyMemory<UhtToken>(Tokens.ToArray()));
#endif
    			PreResovleProperty.SourceName = PropertySettings.SourceName;
    			bNeedsResolution = true;
    			return PreResovleProperty;
            }

            return null;
        }

        private static UhtToken DetermineParsedParameterTypeToken(List<UhtToken> TypeTokens)
        {
            //TODO: Handle any qualifiers also being passed in
            return TypeTokens.First();
        }

        private static String MakeValidParameterName(UhtToken NameToken)
        {
            //TODO: Error handling around malformed name arguments ie empty or illegal names
            return NameToken.Value.ToString();
        }

        /**
         * 
         */
        private static void GenerateParamsStructHeader(UhtParsingScope Scope, UhtModScriptStruct ParamStruct) 
        {
            //Find module to prepare to write param struct to header
            UHTManifest.Module? pluginModule = null;
            foreach (UHTManifest.Module module in Scope.Session.Manifest!.Modules)
            {
                if (String.Equals(module.Name, "ModioUGC", StringComparison.OrdinalIgnoreCase))
                {
                    pluginModule = module;
                    break;
                }
            }

            if (pluginModule == null)
            {
                Scope.Session.LogError($"Cannot find ModioUGC module!");
                return;
            }

            if(ParamStruct.bNeedsResolution)
            {
                //We need to wait to generate the text for this struct so store the module for now
                ParamStruct.ModModule = pluginModule;
                return;
            }

            //Write out the intermediate include file for this struct to be merged later
            WriteStructInclude(pluginModule, ParamStruct);

        }

        /**
         * Generate the contents for a structs header file
         */
        private static string GenerateStructText(UhtScriptStruct ParamStruct)
        {
            {
                using BorrowStringBuilder borrower = new(StringBuilderCache.Big);

                //Begin struct definition
                borrower.StringBuilder.Append("struct Z_Construct_UScriptStruct_"+ ParamStruct.SourceName +"_Statics;");
                borrower.StringBuilder.Append("\r\n");
                borrower.StringBuilder.Append("struct " + ParamStruct.SourceName + " { \r\n");
                borrower.StringBuilder.Append("\tfriend struct Z_Construct_UScriptStruct_" + ParamStruct.SourceName + "_Statics;\r\n");
                borrower.StringBuilder.Append("\tstatic class UScriptStruct* StaticStruct();;\r\n");
                borrower.StringBuilder.Append("\t\r\n");

                //Add members dynamically
                foreach (var Child in ParamStruct.Children)
                {
                    UhtProperty Member = (UhtProperty) Child;
                    if (Member != null)
                    {
                        borrower.StringBuilder.Append("\t");
                        if (Member.PointerType != UhtPointerType.None)
                        {
                            //Forward delcare pointer types
                            borrower.StringBuilder.Append("class ");
                        }
                        Member.AppendFullDecl(borrower.StringBuilder, UhtPropertyTextType.Generic);
                        borrower.StringBuilder.Append(";\r\n");
                        
                    }
                }

                //End struct definition
                borrower.StringBuilder.Append("\t\r\n");
                borrower.StringBuilder.Append("};\r\n");

                return borrower.StringBuilder.ToString();
            }
        }

        /**
         * Write the struct header file to disk
         */
        public static void WriteStructInclude(UHTManifest.Module Module, UhtModScriptStruct ParamStruct)
        {
            string FullPath = Path.Combine(Module!.OutputDirectory, ParamStruct.EngineName + ".mutator.inl");
            ParamStruct.Session.LogInfo($"Output: {FullPath}");
            ParamStruct.Session.FileManager!.WriteOutput(FullPath, GenerateStructText(ParamStruct));
        }

        [UhtKeyword(Extends = UhtTableNames.Class)]
        [SuppressMessage("CodeQuality", "IDE0051:Remove unused private members", Justification = "Attribute accessed method")]
        [SuppressMessage("Style", "IDE0060:Remove unused parameter", Justification = "Attribute accessed method")]
        private static UhtParseResult DECLARE_MUTATOR_EVENT_BLUEPRINTKeyword(UhtParsingScope topScope, UhtParsingScope actionScope, ref UhtToken token)
        {
            return ParseMutatorDeclaration(topScope, token);
        }

        [UhtKeyword(Extends = UhtTableNames.Class)]
        [SuppressMessage("CodeQuality", "IDE0051:Remove unused private members", Justification = "Attribute accessed method")]
        [SuppressMessage("Style", "IDE0060:Remove unused parameter", Justification = "Attribute accessed method")]
        private static UhtParseResult DECLARE_MUTATOR_EVENT_BLUEPRINT_RETURNKeyword(UhtParsingScope topScope, UhtParsingScope actionScope, ref UhtToken token)
        {
            return ParseMutatorDeclaration(topScope, token, true);
        }

        /**
         * Parses the DECLARE_MUTATOR_EVENT_BLUEPRINT macros
         */
        private static UhtParseResult ParseMutatorDeclaration(UhtParsingScope topScope, UhtToken token, bool bHasReturnType = false)
        {
            //Create the new function
#if UE_5_7_OR_LATER
            UhtFunction function = new(topScope.HeaderFile, topScope.HeaderParser.GetNamespace(), topScope.ScopeType, token.InputLine);
#elif UE_5_5_OR_LATER
            UhtFunction function = new(topScope.HeaderFile, topScope.ScopeType, token.InputLine);
#else
            UhtFunction function = new(topScope.ScopeType, token.InputLine);
#endif

            //Set up the function specifiers so the function can be overridden as expected
            function.MacroLineNumber = topScope.TokenReader.InputLine;
            function.FunctionType = UhtFunctionType.Function;
            function.FunctionFlags |= EFunctionFlags.Native;
            function.FunctionFlags |= EFunctionFlags.Event;
            function.FunctionFlags |= EFunctionFlags.BlueprintEvent;
            function.FunctionExportFlags |= UhtFunctionExportFlags.ImplFound;

            //Make sure the functions access is the same as where the macro is used
            switch (topScope.AccessSpecifier)
            {
                case UhtAccessSpecifier.Public:
                    function.FunctionFlags |= EFunctionFlags.Public;
                    break;

                case UhtAccessSpecifier.Protected:
                    function.FunctionFlags |= EFunctionFlags.Protected;
                    break;

                case UhtAccessSpecifier.Private:
                    function.FunctionFlags |= EFunctionFlags.Private | EFunctionFlags.Final;
                    break;
            }

            //Parse the macro parameters
            List<UhtToken> tokens = new List<UhtToken>();
            topScope.TokenReader.Require('(');

            //Parse name
            UhtToken Name = topScope.TokenReader.GetToken();

            //Macro should just contain the name as it's argument
            topScope.TokenReader.Require(')');

            //Recreating the property type would cause a redefinition to create a new token to look up the param type defined by the DEFINE_MUTATOR
            UhtToken T = new UhtToken(UhtTokenType.Identifier, 0, 0, 0, 0, "F" + Name.Value.ToString() + "_Params");
            List<UhtToken> Tokens = new List<UhtToken>() { T };

            //Add a parameter to the new param struct type to the parameter list of the function
            UhtPropertySettings StructParamSettings = new();
            StructParamSettings.PropertyCategory = UhtPropertyCategory.RegularParameter;
            StructParamSettings.SourceName = "Params";
            StructParamSettings.EngineName = "Params";
            StructParamSettings.Outer = function;

            //Add the struct property to get resolved after the parsing phase
#if UE_5_6_OR_LATER
            //We need to add a terminating symbol to the definition
            UhtToken DeclarationTerminationToken = new UhtToken(UhtTokenType.Symbol);
            DeclarationTerminationToken.Value = ";";
            Tokens.Add(DeclarationTerminationToken);

            IUhtTokenReader ParamReplayReader = new UhtTokenReplayReader(topScope.HeaderFile, topScope.HeaderFile.Data.Memory, new ReadOnlyMemory<UhtToken>(Tokens.ToArray()), UhtTokenType.EndOfDeclaration);
            var TypeTokens = UhtTypeTokens.Gather(ParamReplayReader);
            StructParamSettings.TypeTokens = new(TypeTokens, TypeTokens.Segments.Length - 1);
            UhtPreResolveProperty ParamProp = new UhtPreResolveProperty(StructParamSettings);
#else
            UhtPreResolveProperty ParamProp = new UhtPreResolveProperty(StructParamSettings, new ReadOnlyMemory<UhtToken>(Tokens.ToArray()));
#endif
            ParamProp.PropertyFlags |= EPropertyFlags.Parm;
            ParamProp.PropertyCaps |= UhtPropertyCaps.IsParameterSupportedByBlueprint;
            function.AddChild(ParamProp);

            //If we have a return type handle that here
            if(bHasReturnType)
            {
                //Return type property settings
                UhtPropertySettings ReturnPropertySettings = new();
                ReturnPropertySettings.PropertyCategory = UhtPropertyCategory.RegularParameter;
                ReturnPropertySettings.PropertyFlags |= EPropertyFlags.Parm | EPropertyFlags.OutParm | EPropertyFlags.ReturnParm;
                ReturnPropertySettings.SourceName = "ReturnValue";
                ReturnPropertySettings.EngineName = ReturnPropertySettings.SourceName;
                ReturnPropertySettings.Outer = function;

#if UE_5_6_OR_LATER
                ReturnPropertySettings.TypeTokens = new(TypeTokens, TypeTokens.Segments.Length - 1);
                UhtPreResolveProperty ReturnProp = new UhtPreResolveProperty(ReturnPropertySettings);
#else
                UhtPreResolveProperty ReturnProp = new UhtPreResolveProperty(ReturnPropertySettings, new ReadOnlyMemory<UhtToken>(Tokens.ToArray()));
#endif
                function.AddChild(ReturnProp);
            }

            //Fill out details for the thunk based on the implementation function
            function.SourceName = Name.Value.ToString();
            function.EngineName = Name.Value.ToString();
            function.MarshalAndCallName = Name.Value.ToString();
            function.UnMarshalAndCallName = "exec" + Name.Value.ToString();
            function.CppImplName = Name.Value.ToString() + "_Implementation";

            //Set up some meta data to make the generated function match out standards
            function.MetaData.Add(UhtNames.Category, "mod.io|Mutators|Events");
            function.MetaData.Add(UhtNames.DisplayName, function.SourceName);
            function.MetaData.Add("ForceAsFunction", "");

            //Add the thunk to get generated
            function.Outer?.AddChild(function);

            return UhtParseResult.Handled;
        }
    }

    [UnrealHeaderTool]
    public static class Exporter
    {
        [UhtExporter(Name = "MutatorEventHeaderConsolidator", ModuleName = "ModioUGC", Options = UhtExporterOptions.Default)]
        public static void Generate(IUhtExportFactory factory)
        {
            //Take all the *.mutator.inl files generated during parsing and combine them into a single header to make things easier to use
            {
                using BorrowStringBuilder borrower = new(StringBuilderCache.Big);

                borrower.StringBuilder.Append("#pragma once\r\n");
                borrower.StringBuilder.Append("\r\n");

                //Find all generated mutator includes
                var HeadersToConsolidate = Directory.EnumerateFiles(factory.PluginModule!.OutputDirectory, "*.mutator.inl");
                foreach(string Header in HeadersToConsolidate)
                {
                    //Combine the contents of the header
                    borrower.StringBuilder.Append($"\r\n");
                    borrower.StringBuilder.Append(File.ReadAllText(Header));

                    //Clean up intermediate struct definition includes so old event definitions don't hang around and cause code bloat
                    File.Delete(Header);
                }

                //Save the new monolithic params header
                string FullPath = Path.Combine(factory.PluginModule!.OutputDirectory, "MutatorEvents.generated.inl");
                factory.CommitOutput(FullPath, borrower.StringBuilder.ToString());
            }
        }
    }
}
