#include <semcore.h>
#include <filesystem>

using namespace std;
static SemLearner semLearn;
vector<Test*> SemLearnerTest::tests;
static string semsDirectory = "";
static unsigned semsDirNo   = 0;

#ifdef _C_EXE_

void Init(char* symSumPath, char* cgDotPath)
{
    semLearn.init (symSumPath, cgDotPath);
    return;
}

void GetSemSummary(char* inDot)
{
    vector<string> semVec = semLearn.getSemantics (inDot);
    return;
}

SemLearner* GetSemLearner ()
{
    return &semLearn;
}

#else
#include <filesystem>
#include <Python.h>

static void saveDirectory(const string& filePath="None") 
{
    if (filePath == "None")
    {
        filesystem::path current = filesystem::current_path();
        semsDirectory = current.string() + "/fuzz/in/semantic";
    }
    else
    {
        filesystem::path path(filePath);
        string directory = path.parent_path().string();
        semsDirectory = directory + "/fuzz/in/semantic";
    }

    cout<<"@@saveDirectory: "<<semsDirectory<<endl;
    return;
}

static PyObject *Init(PyObject *self, PyObject *args)
{
    char* symSumPath;
    char* cgDotPath;

    if (!PyArg_ParseTuple(args, "ss", &symSumPath, &cgDotPath))
	{
        Py_RETURN_NONE;
	}
	
    saveDirectory (symSumPath);
    if (strcmp(symSumPath, "None") != 0 && strcmp(cgDotPath, "None") != 0)
    {
        semLearn.init (symSumPath, cgDotPath);
    }
    
    Py_RETURN_NONE;
}

static PyObject *GetFuncList(PyObject *self, PyObject *args)
{
    vector<string> funcList;
	
    funcList = semLearn.getFuncList ();
    PyObject *pyList = PyList_New(funcList.size());
    if (!pyList) 
    {
        PyErr_SetString(PyExc_MemoryError, "Unable to allocate memory for Python list.");
        Py_RETURN_NONE;
    }

    for (size_t i = 0; i < funcList.size(); ++i) 
    {
        PyObject *pyStr = PyUnicode_FromString(funcList[i].c_str());
        if (!pyStr) 
        {
            Py_DECREF(pyList);
            PyErr_SetString(PyExc_ValueError, "Unable to convert C++ string to Python string.");
            Py_RETURN_NONE;
        }
        PyList_SET_ITEM(pyList, i, pyStr);
    }

    return pyList;
}


static PyObject *getFuncSummary(PyObject *self, PyObject *args)
{
    char* funcName;

    if (!PyArg_ParseTuple(args, "s", &funcName))
	{
        Py_RETURN_NONE;
	}

    std::string funcSummary = semLearn.getFuncSummary (funcName);
    return Py_BuildValue("s", funcSummary.c_str());
}

static PyObject *setFuncSemantic (PyObject *self, PyObject *args)
{
    char* funcName;
    char* funcSemantics;

    if (!PyArg_ParseTuple(args, "ss", &funcName, &funcSemantics))
	{
        Py_RETURN_NONE;
	}

    semLearn.setFuncSemantic (funcName, funcSemantics);
    Py_RETURN_NONE;
}

static PyObject *getFuncTokenNum(PyObject *self, PyObject *args)
{
    char* funcName;

    if (!PyArg_ParseTuple(args, "s", &funcName))
	{
        Py_RETURN_NONE;
	}

    unsigned TokenNum = semLearn.getFuncTokenNum (funcName);
    return Py_BuildValue("i", TokenNum);
}

static PyObject *setFuncTokenNum (PyObject *self, PyObject *args)
{
    char* funcName;
    unsigned funcTokenNum;

    if (!PyArg_ParseTuple(args, "si", &funcName, &funcTokenNum))
	{
        Py_RETURN_NONE;
	}

    semLearn.setFuncTokenNum (funcName, funcTokenNum);
    Py_RETURN_NONE;
}

static PyObject *saveSummary (PyObject *self, PyObject *args)
{
    semLearn.saveSummary (); 
    Py_RETURN_NONE;
}

static PyObject *GetSemanticPaths (PyObject *self, PyObject *args)
{
    char* inDot;
    unsigned randomGen = false;
    vector<string> semVec;

    if (!PyArg_ParseTuple(args, "sp", &inDot, &randomGen))
	{
        Py_RETURN_NONE;
	}

    cout<<"@@randomGen = "<<randomGen<<endl;
	
    semVec = semLearn.getSemantics (inDot, randomGen);   
    PyObject *pyList = PyList_New(semVec.size());
    if (!pyList) 
    {
        PyErr_SetString(PyExc_MemoryError, "Unable to allocate memory for Python list.");
        Py_RETURN_NONE;
    }

    for (size_t i = 0; i < semVec.size(); ++i) 
    {
        PyObject *pyStr = PyUnicode_FromString(semVec[i].c_str());
        if (!pyStr) 
        {
            Py_DECREF(pyList);
            PyErr_SetString(PyExc_ValueError, "Unable to convert C++ string to Python string.");
            Py_RETURN_NONE;
        }
        PyList_SET_ITEM(pyList, i, pyStr);
    }

    return pyList;
}


static inline string getSemDir ()
{
    while (true)
    {
        string semDir = semsDirectory + to_string (semsDirNo);
        if (filesystem::exists(semDir))
        {
            if (!filesystem::exists(semDir + "/read_done"))
            {
                return semDir;
            }
            else
            {
                semsDirNo++;
                continue;
            }
        }

        filesystem::create_directory(semDir);
        return semDir; 
    }
}


static unsigned totalTestNum = 0;
map <unsigned long, unsigned> testHashes;
static inline unsigned long computeHash(char *testContent)
{
    unsigned long hash = 5381;

    char *c = testContent;
    while (*c != 0)
    {
        hash = ((hash << 5) + hash) + *c;
        c++;
    }
    return hash;
}

static inline bool checkAndSaveTestHash (char *testContent, unsigned len, unsigned long& hv)
{
    hv = computeHash (testContent);
    auto itr = testHashes.find (hv);
    if (itr != testHashes.end ())
    {
        if (itr->second == len)
        {
            fprintf (stderr, "@@checkAndSaveTestHash -->[%lu/%u] TEST:%s exists\n", 
                     testHashes.size(), totalTestNum, testContent);
            return false;
        }
    }
    
    testHashes [hv] = len;
    return true;
}

static inline string toHex (unsigned long hash)
{
    ostringstream oss;
    oss << hex << hash;

    return oss.str();
}

static PyObject *saveSemTest (PyObject *self, PyObject *args)
{
    char* testName;
    char* testContent;

    if (!PyArg_ParseTuple(args, "ss", &testName, &testContent)) 
    {
        return Py_BuildValue("s", "");
    }
    
    totalTestNum++;
    unsigned long hash = 0;
    if (checkAndSaveTestHash (testContent, strlen (testContent), hash) == false)
    {
        return Py_BuildValue("s", toHex (hash).c_str());
    }

    string semDir   = getSemDir ();
    string filename = semDir + "/" + testName;
    ofstream file(filename);
    if (!file.is_open()) 
    {
        PyErr_SetString(PyExc_IOError, "Failed to open file for writing.");
        return Py_BuildValue("s", toHex (hash).c_str());
    }
    file << testContent;
    file.close();

    printf("[%lu]Saved test '%s'\n", testHashes.size(), filename.c_str());
    return Py_BuildValue("s", toHex (hash).c_str());
}

static PyObject *saveBinaryTest (PyObject *self, PyObject *args)
{
    char* testName;
    PyObject *binaryObj;

    if (!PyArg_ParseTuple(args, "sO", &testName, &binaryObj)) 
    {
        return Py_BuildValue("s", "");
    }

    if (!PyBytes_Check(binaryObj)) 
    {
        PyErr_SetString(PyExc_TypeError, "Expected a bytes object for binary data");
        return NULL;
    }

    char *data;
    Py_ssize_t length;
    if (PyBytes_AsStringAndSize(binaryObj, &data, &length) < 0) 
    {
        return NULL;
    }
    
    totalTestNum++;
    unsigned long hash = 0;
    if (checkAndSaveTestHash (data, length, hash) == false)
    {
        return Py_BuildValue("s", toHex (hash).c_str());
    }

    string semDir   = getSemDir ();
    string filename = semDir + "/" + testName;
    ofstream file(filename, ios::binary);
    if (!file.is_open()) 
    {
        PyErr_SetString(PyExc_IOError, "Failed to open file for writing.");
        return Py_BuildValue("s", toHex (hash).c_str());
    }
    file.write(data, length);
    file.close();

    printf("[%lu]Saved Binary test '%s'\n", testHashes.size(), filename.c_str());
    return Py_BuildValue("s", toHex (hash).c_str());
}

static PyMethodDef SemcoreMethods[] = 
{
    {"Init",             Init,             METH_VARARGS, "init before use"},
    {"GetFuncList",      GetFuncList,      METH_VARARGS, "get list of all the functions"},
    {"getFuncSummary",   getFuncSummary,   METH_VARARGS, "get summary for a given function with name"},
    {"setFuncSemantic",  setFuncSemantic,  METH_VARARGS, "set semantics for a given function with name"},
    {"getFuncTokenNum",  getFuncTokenNum,  METH_VARARGS, "get token num for a given function with name"},
    {"setFuncTokenNum",  setFuncTokenNum,  METH_VARARGS, "set token num for a given function with name"},
    {"saveSummary",      saveSummary,      METH_VARARGS, "set semantics for a given function with name"},
    
    {"GetSemanticPaths", GetSemanticPaths, METH_VARARGS, "get semantic paths for a given sub-callgraph (.dot)"},
    {"saveSemTest",      saveSemTest,      METH_VARARGS, "save semantic test for fuzzing"},
    {"saveBinaryTest",   saveBinaryTest,   METH_VARARGS, "save semantic test  (binary) for fuzzing"},

    {NULL, NULL, 0, NULL} 
};

static struct PyModuleDef ModSemcore =
{
    PyModuleDef_HEAD_INIT,
    "semcore",     /* name of module */
    "",            /* module documentation, may be NULL */
    -1,            /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
    SemcoreMethods
};

PyMODINIT_FUNC PyInit_semcore(void)
{
    return PyModule_Create(&ModSemcore);
}

#endif