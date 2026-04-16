#ifndef _C_EXE_
#include <Python.h>
#endif
#include <time.h>
#include <semgdump.h>
#include <Queue.h>
#include <ctrace/Event.h>
#include <dump.h>

#if 0
#define READSHM_DEBUG(format, ...) fprintf(stderr, "@@@ <ReadShm> -> " format, ##__VA_ARGS__)
#else
#define READSHM_DEBUG(format, ...)
#endif

static SemgDump *SemGD = NULL;
static bool initDone = false;
static bool isExit   = false;

#define SOFT_DEBUG (getenv("SOFTDEBUG") != NULL)

void* GenDotMain (void *Para)
{
    CSeqQueue *CsQueue = &SemGD->CsQueue;
    while (!isExit)
    {  
        while (CsQueue->GetSfQueueSize () > 0)
        {
            CSBuf *CSB = CsQueue->PopSfQueue ();
            string DotName = SemGD->GenDot (CSB);

            if (SOFT_DEBUG) SemGD->ShowCSeqs (CSB, DotName);
            delete CSB;
        }
    }

    READSHM_DEBUG("--> Exit from GenDotMain ....\r\n");
    delete SemGD;
    pthread_exit ((void*)0);
}

void* ReadQueueMain (void *Para)
{
    /* init event queue */
    InitQueue(MEMMOD_SHARE);

    CSBuf *CSB = NULL;
    while (!isExit)
    {
        CSB = new CSBuf ();
        assert (CSB != NULL);

        while (!CSB->isUpBound ())
        {
            QNode *QN = ::FrontQueue ();
            if (QN == NULL)
            {
                continue;
            }

            if (QN->TrcKey == 0)
            {
                ::OutQueue (QN);
                continue;
            }

            if (QN->IsReady == FALSE)
            {
                if (QN->TimeStamp != 0)
                {
                    unsigned CurTime  = time (NULL);
                    unsigned TimeCost = CurTime - QN->TimeStamp;
                    if (TimeCost >= 300 || QN->TrcKey == 0)
                    {
                        //fprintf(stderr, "(time (NULL) - QN->TimeStamp (%u - %u) = %u, QN->IsReady = %u, QN->TrcKey = %u \r\n", 
                        //        CurTime, QN->TimeStamp, TimeCost, QN->IsReady, QN->TrcKey);
                        ::OutQueue (QN);
                    }
                }

                continue;
            }

            ObjValue *OV = (ObjValue *)QN->Buf;
            READSHM_DEBUG("Get Funcction: [%u][%u]%s\r\n", CSB->Size(), OV->Value, SemGD->GetFuncName((unsigned)OV->Value).c_str());      
            CSB->AddCSeq (CallSeq (QN->TrcKey, OV->Value));
            ::OutQueue (QN);
        }

        if (CSB->Size () > 0)
        {
            READSHM_DEBUG("--> push to CsQueue: %u\r\n", CSB->Size ());
            SemGD->CsQueue.PushSfQueue (CSB);
        }
        else
        {
            delete CSB;
        }
    }

    READSHM_DEBUG ("--> Exit from ReadQueueMain ....\r\n");
    delete SemGD;
    pthread_exit ((void*)0);
}


void StartReadShm ()
{
    if (SemGD == NULL)
    {
        fprintf (stderr, "StartReadShm: DG is NULL, please init first!\r\n");
        exit (-1);
    }

    pthread_t Tid = 0;
    int Ret = pthread_create(&Tid, NULL, ReadQueueMain, NULL);
    if (Ret != 0)
    {
        fprintf (stderr, "pthread_create for ReadQueueMain fail, Ret = %d\r\n", Ret);
        exit (0);
    }

    Ret = pthread_create(&Tid, NULL, GenDotMain, NULL);
    if (Ret != 0)
    {
        fprintf (stderr, "pthread_create for GenDotMain fail, Ret = %d\r\n", Ret);
        exit (0);
    }

    return;
}


void StartReadShmCovF(std::set<std::string> &covFs)
{
    // Initialize shared-memory event queue
    InitQueue(MEMMOD_SHARE);

    while (::QueueSize() > 0) {
        QNode *QN = ::FrontQueue();
        if (QN == nullptr) {
            continue;
        }

        ObjValue *OV = reinterpret_cast<ObjValue *>(QN->Buf);
        if (OV == nullptr) {
            continue;
        }

        unsigned FuncID = OV->Value;
        if (FuncID != 0)
        {
            std::string fName = SemGD->GetFuncName(FuncID);
            //std::cout << "::fName = " << fName << std::endl;
            if (!fName.empty()) 
            {
                covFs.insert(fName);
            }      
        }

        ::OutQueue (QN);        
    }

    return;
}


char* PopCallGraph ()
{
    std::string DotFile = "";

    if (SemGD != NULL && SemGD->DtQueue.GetSfQueueSize () != 0)
    {
        DotFile = SemGD->DtQueue.PopSfQueue ();
    }

    return strdup (DotFile.c_str());
}

void Init (char* DotFile, char* FIdFile, char* StCgDot, unsigned MinSeq)
{
    SemGD = new SemgDump (string (DotFile), string (FIdFile));
    if (SemGD == NULL)
    {
        fprintf (stderr, "Init DotGen Graph fail!\r\n");
        exit (0);
    }

    if (StCgDot != NULL)
    {
        SemGD->InitSemgWithDot (string (StCgDot));
    }

    initDone = true;
}

void ConvertDot (char* StCgDot, char* FIDsFile)
{
    SemGD = new SemgDump ("CCallGraph.dot", string (FIDsFile));
    if (SemGD == NULL)
    {
        fprintf (stderr, "Init DotGen Graph fail!\r\n");
        exit (0);
    }

    SemGD->ConvertDot (string (StCgDot));
    return;
}

#ifndef _C_EXE_
static PyObject *Init(PyObject *self, PyObject *args)
{
    char* DotFile = NULL;
    char* FIdFile = NULL;
    char* DotGraph = NULL;

    if (!PyArg_ParseTuple(args, "sss", &DotFile, &FIdFile, &DotGraph))
	{
        Py_RETURN_NONE;
	}
	
	SemGD = new SemgDump (string (DotFile), string (FIdFile));
    if (SemGD == NULL)
    {
        fprintf (stderr, "Init SemgDump fail -> DotFile = %p, FIdFile = %p\r\n", DotFile, FIdFile);
        Py_RETURN_NONE;
    }

    if (DotGraph != NULL)
    {
        SemGD->InitSemgWithDot (string (DotGraph));
    }

    initDone = true;
    Py_RETURN_NONE;
}

static PyObject *Exit(PyObject *self, PyObject *args)
{
    isExit = true;
    sleep (1);
    exit (0);
    Py_RETURN_NONE;
}

static PyObject *WaitInitDone(PyObject *self, PyObject *args)
{
    while (initDone == false)
    {
        sleep (1);
    }

    printf ("@@WaitInitDone: init success!\n");
    Py_RETURN_NONE;
}

static PyObject *StartDumping (PyObject *self, PyObject *args)
{
	StartReadShm ();
    Py_RETURN_NONE;
}

static PyObject *PopCallGraph(PyObject *self, PyObject *args) 
{
    std::string DotFile = "";

    if (SemGD != NULL && SemGD->DtQueue.GetSfQueueSize () > 0)
    {
        DotFile = SemGD->DtQueue.PopSfQueue ();
    }

    return Py_BuildValue("s", DotFile.c_str());
}


static PyObject *PopFuncGroup(PyObject *self, PyObject *args) 
{
    std::string FGDot = "";

    if (SemGD != NULL && SemGD->IsLandsQ.GetSfQueueSize () > 0)
    {
        FGDot = SemGD->IsLandsQ.PopSfQueue ();   
    }

    return Py_BuildValue("s", FGDot.c_str());
}

static PyObject *GetCoveredFuncs(PyObject *self, PyObject *args) 
{
    // If SemGD is not initialized, return an empty list
    if (SemGD == nullptr) {
        return PyList_New(0);
    }

    std::set<std::string> covFs;
    StartReadShmCovF(covFs);

    PyObject *py_list = PyList_New(static_cast<Py_ssize_t>(covFs.size()));
    if (!py_list) {
        Py_RETURN_NONE;
    }

    Py_ssize_t i = 0;
    for (const auto &fname : covFs) {
        PyObject *py_str = PyUnicode_FromString(fname.c_str());
        if (!py_str) {
            Py_DECREF(py_list);
            Py_RETURN_NONE;
        }
        // PyList_SET_ITEM steals reference to py_str
        PyList_SET_ITEM(py_list, i++, py_str);
    }

    return py_list;
}

static PyObject *LoadGraph(PyObject *self, PyObject *args)
{
    const char *dotFile = nullptr;
    if (!PyArg_ParseTuple(args, "s", &dotFile)) {
        Py_RETURN_NONE;
    }

    SemGraph semg;
    SemgDotParser semDtp(dotFile);
    semDtp.Dot2Graph(semg);

    std::set<std::string> fnames;
    for (auto itr = semg.begin(); itr != semg.end(); ++itr) {
        auto *node = itr->second;
        std::string fname = node->GetFName();
        fnames.insert(fname);
    }

    PyObject *py_list = PyList_New(static_cast<Py_ssize_t>(fnames.size()));
    if (!py_list) {
        Py_RETURN_NONE;
    }

    Py_ssize_t i = 0;
    for (const auto &f : fnames) {
        PyObject *py_str = PyUnicode_FromString(f.c_str());
        if (!py_str) {
            Py_DECREF(py_list);
            Py_RETURN_NONE;
        }
        PyList_SET_ITEM(py_list, i++, py_str);
    }

    return py_list;
}


static PyMethodDef SemDumpMethods[] = 
{
    {"Init", Init, METH_VARARGS, "init before use"},
    {"Exit", Exit, METH_VARARGS, "exit the execution"},
    {"WaitInitDone", WaitInitDone, METH_VARARGS, "wait till the init status is set"},
    {"StartDumping", StartDumping, METH_VARARGS, "read data from shared queue and dumping"},
    {"PopCallGraph", PopCallGraph, METH_VARARGS, "pop one created call graph"},
    {"PopFuncGroup", PopFuncGroup, METH_VARARGS, "pop one function group"},
    {"GetCoveredFuncs", GetCoveredFuncs, METH_VARARGS, "get all covered functions"},
    {"LoadGraph", LoadGraph, METH_VARARGS, "load graph from dot"},
    {NULL, NULL, 0, NULL} 
};

static struct PyModuleDef SemgdMod =
{
    PyModuleDef_HEAD_INIT,
    "semgdump",  /* name of module */
    "",          /* module documentation, may be NULL */
    -1,          /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
    SemDumpMethods
};

PyMODINIT_FUNC PyInit_semgdump(void)
{
    return PyModule_Create(&SemgdMod);
}
#endif