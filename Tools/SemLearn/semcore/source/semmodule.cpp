#include <semmodule.h>

using namespace std;


void ModuleSem::initModule(const string& semJsonPath) 
{
    // save the path to default
    defSemJsonPath = semJsonPath;

    try 
    {
        ifstream file(semJsonPath);
        if (!file.is_open()) 
        {
            throw runtime_error("Could not open file");
        }

        json parsedJson;
        file >> parsedJson;

        if (parsedJson.contains("globals")) 
        {
            for (const auto& global : parsedJson["globals"])
            {
                processGlobal(global);
            }
            printf ("@@initModule: totally load globals: %-60lu\r\n", ModuleSem::globalsMap.size());
        }

        if (parsedJson.contains("functions")) 
        {
            for (const auto& func : parsedJson["functions"]) 
            {
                processFunction(func);
            }
            printf ("@@initModule: totally load functions: %-60lu\r\n", funcId2FuncSem.size());
        }
    } 
    catch (const nlohmann::json::exception& e) 
    {
        cerr << "JSON Parsing Error: " << e.what() << "\n";
        exit (0);
    } 
    catch (const exception& e) 
    {
        cerr << "Error: " << e.what() << "\n";
        exit (0);
    }

    //unitTest ();
}

void ModuleSem::exportSemSummary (string outputFilePath) 
{
    if (outputFilePath == "")
    {  
        string dir, name;
        getFilePathName (defSemJsonPath, dir, name);
        outputFilePath = dir + "/export_" + name;
    }

    try 
    {
        json outputJson;

        // Export globals
        if (!globalsMap.empty()) 
        {
            json globalsArray = json::array();
            for (const auto& [name, globalVar] : globalsMap) 
            {
                json globalJson = {
                    {"name", globalVar.name},
                    {"type", globalVar.type},
                    {"value", globalVar.value}
                };
                globalsArray.push_back(globalJson);
            }
            outputJson["globals"] = globalsArray;
        }

        // Export functions
        if (!funcId2FuncSem.empty()) 
        {
            json functionsArray = json::array();
            for (const auto& [id, funcSem] : funcId2FuncSem) 
            {
                json funcJson = {
                    {"id", std::to_string(funcSem->FId)},
                    {"name", funcSem->FName},
                    {"input", funcSem->FInputs},
                        //{"output", ""}
                };
                    
                if (funcSem->Semantics != "")
                {
                    funcJson["semantic"] = funcSem->Semantics;
                }

                if (funcSem->Tokens != 0)
                {
                    funcJson["tokens"] = funcSem->Tokens;
                }

                // Export function calls
                json callsArray = json::array();
                for (const auto& call : funcSem->FCalls) 
                {
                    json callJson = {
                        {"callee", call.callees},
                        {"expr", call.calleeExpr},
                        {"conditions", call.callConditions}
                    };
                    callsArray.push_back(callJson);
                }
                funcJson["calls"] = callsArray;

                // Export function paths
                json pathsArray = json::array();
                for (const auto& path : funcSem->Paths) 
                {
                    json argOutJson;
                    for (const auto &argOutput : path.argOutputs) 
                    {
                        argOutJson[argOutput.first] = argOutput.second;
                    }

                    json pathJson = {
                        {"arg_outputs", argOutJson},
                        {"conditions", path.pathConditions},
                        {"return", path.returnExpr}
                    };
                    pathsArray.push_back(pathJson);
                }
                funcJson["paths"] = pathsArray;

                functionsArray.push_back(funcJson);
            }
            outputJson["functions"] = functionsArray;
        }

        // Write JSON to file
        std::cout << "@@exportToJson: Exporting data to JSON file: " << outputFilePath << "\n";

        std::ofstream file(outputFilePath, std::ios::out | std::ios::trunc);
        if (!file.is_open()) 
        {
            throw std::runtime_error("@@exportToJson: Failed to open output file for writing");
        }

        file << outputJson.dump(4) << "\n";
        file.close();
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "@@exportToJson: Error exporting to JSON: " << e.what() << "\n";
    }
}


string ModuleSem::setGlobalVarValue (const string& strExpr) const
{
    string result;
    size_t pos = 0;

    while (pos < strExpr.size()) 
    {
        size_t start = strExpr.find(".str", pos); // Find the next occurrence of ".str"
        if (start == string::npos) 
        {
            // Append the rest of the strExpr if no ".str" is found
            result.append(strExpr, pos, string::npos);
            break;
        }
        
        // Append text before ".str"
        result.append(strExpr, pos, start - pos);

        // Find the end of the ".str.*" pattern
        size_t end = start + 4; // Start after ".str"
        while (end < strExpr.size() && (isdigit(strExpr[end]) || strExpr[end] == '.')) 
        {
            ++end;
        }

        // Extract the substring like ".str.41"
        string gName  = strExpr.substr(start, end - start);
        string gValue = getGlobalValue (gName);
        if (gValue != "") 
        {
            result.append("\"" + gValue + "\"");
        } 
        else 
        {
            result.append(gName);
        }

        pos = end;
    }

    return result;
}


void ModuleSem::mergeGlobalVar(const string& gvName, GlobalVariable globalVar)
{
    if (getGlobalValue (gvName) != "")
    {
        return;
    }

    cout<<"@@mergeGlobalVar: add global variable: "<<gvName<<" -> "<<globalVar.value<<endl;
    globalsMap[gvName] = globalVar;
    return;
}

void ModuleSem::unitTest ()
{
    test ("!(arg0<2) & !(strcmp(arg1,.str.41)==0) & !(strcmp(arg1,.str.42)==0) & (arg1==45)"); 
    test ("fprintf(stdout,.str.11,arg2)@type(i32)");
    test ("fprintf(arg0,.str.106.1392,5027,.str.234.1488)@type(i32)");
    test ("xmlSchemaPErrFull(arg0,arg2,3036,2,i8*_undef,i32_undef,null,.str.231.3436,null,i32_undef,.str.230.3437,null,.str.231.3436)");
}