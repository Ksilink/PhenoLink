#include "Python.h"
#include "pyerrors.h"
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include <numpy/arrayobject.h>



#include <QSettings>
#include "checkoutpythonpluginshandler.h"

#include "Core/checkoutprocessplugininterface.h"
#include "Core/checkoutprocess.h"

PythonProcess* current_Process = 0;

QStringList pyObjToStringList(PyObject* ob);

static PyObject *CheckoutErrors_plugin = NULL;
static PyObject *checkout_instance = NULL ;

PyObject* py_Image2D = 0x0;
PyObject* py_TimeImage = 0x0;
PyObject* py_StackedImage = 0x0;
PyObject* py_TimeStackedImage = 0x0;
PyObject* py_ImageXP = 0x0;
PyObject* py_TimeImageXP = 0x0;
PyObject* py_StackedImageXP = 0x0;
PyObject* py_TimeStackedImageXP = 0x0;

PyObject* py_Int = 0x0;
PyObject* py_Double = 0x0;

PyObject* imageTypes = 0x0;

QMutex python_mutex;

PyThreadState* thread_state = 0x0;

class PythonProcess: public CheckoutProcessPluginInterface
{
public:

    PythonProcess(QString script_path): CheckoutProcessPluginInterface(),
        _script_path(script_path),
        initState(0), cloned(false)
    {
    }

    virtual void read(const QJsonObject &json)
    {
        CheckoutProcessPluginInterface::read(json);
    }



    virtual bool isPython()  { return true; }

    ~PythonProcess()
    {
        //    if (!cloned)  // Only free the temporary memory if the data was cloned
        //      {
        //    foreach (RegistrableParent* d, _parameters.values())
        //      delete d;

        //    foreach (RegistrableParent* d, _results.values())
        //      delete d;
        _parameters.clear();
        _results.clear();
        //      }
        qDebug() << "Deleting python process";

    }

    virtual CheckoutProcessPluginInterface* clone()
    {
        PythonProcess* r = new PythonProcess(_script_path);

        r->path         = this->path;

        foreach (RegistrableParent* p, _parameters)
            r->_parameters[p->tag()] = p->dup();
        foreach (RegistrableParent* p, _results)
            r->_results[p->tag()] = p->dup();

        r->cloned       = true;

        return r;
    }

#define PyGetType(obj, TYPE, i, j) *(TYPE *)PyArray_GETPTR2((PyArrayObject*)(obj), (i), (j))


    void description(QString path, QStringList authors,  QString comment )
    {
        CheckoutProcessPluginInterface::description(path, authors, comment);
        initState |= 1;
    }

    virtual void exec()
    {
        qDebug() << "Python Plugin exec called";
        //    PyEval_AcquireLock() ; // nb: get the GIL

        //    PyThreadState* pThreadState = Py_NewInterpreter() ;

        //    assert( pThreadState != NULL ) ;
        //    PyEval_ReleaseThread( pThreadState ) ; // nb: this also releases the GIL

        QMutexLocker locker(&python_mutex);
        //    PyThreadState* thread_state = Py_NewInterpreter() ;

        //    PyInterpreterState* interpreterState = thread_state->interp;
        //    PyThreadState* state = PyThreadState_New(interpreterState);
        //    PyEval_RestoreThread(state);
        //    PyThreadState* saved = PyEval_SaveThread();

        //    PyGILState_STATE gstate;

        //    gstate = PyGILState_Ensure();

        if (!checkout_instance) { qDebug() << "Python instance failed"; /*PyGILState_Release(gstate); */ return; }

        qDebug() << "Python Plugin sane state";
        //    PyEval_AcquireThread( pThreadState ) ;

        PyRun_SimpleString("import time");
        PyRun_SimpleString("import sys");
        //    PyRun_SimpleString(QString("sys.stdout = sys.stderr = open(\"%1\", \"w\")").arg(QString(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first() + "/Checkout_PluginPython_%1Log.txt")).arg((int)QThread::currentThreadId()).toLatin1());
        PyRun_SimpleString(QString("sys.stdout = sys.stderr = open(\"%1\", \"w\")").arg(QString(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first() + "/Checkout_PluginPython_Log.txt")).toLatin1());

        PyObject* mod = PyModule_GetDict(checkout_instance);
        PyDict_SetItemString(mod, "state", Py_BuildValue("s", "run"));

        qDebug() << "Preparing data in python env";

        qDebug() << _parameters.size() << _results.size();

        foreach (RegistrableParent* r, _parameters.values())
        {
            if (dynamic_cast<RegistrableImageParent*>(r))
            {
                QJsonArray arr = _callParams["Parameters"].toArray();
                for (int i = 0; i < arr.size(); ++i)
                    if (arr.at(i).toObject()["Tag"].toString() == r->tag())
                    { // Got the object related to our tag!
                        QJsonDocument doc(arr.at(i).toObject());
                        char* str = strdup(doc.toJson());
                        PyDict_SetItemString(mod, r->tag().toLatin1(), Py_BuildValue("s", str));
                        free(str);
                    }
            }
            else
            { // Int or Double type

                if (dynamic_cast<Registrable<int>*>(r))
                {
                    Registrable<int>* rr = dynamic_cast<Registrable<int>*>(r);
                    PyDict_SetItemString(mod, rr->tag().toLatin1(), Py_BuildValue("i",rr->value()));
                }

                if (dynamic_cast<Registrable<double>*>(r))
                {
                    Registrable<double>* rr = dynamic_cast<Registrable<double>*>(r);
                    PyDict_SetItemString(mod, rr->tag().toLatin1(), Py_BuildValue("d",rr->value()));
                }
                if (dynamic_cast<RegistrableEnum<int>*>(r))
                {
                    RegistrableEnum<int>* rr = dynamic_cast<RegistrableEnum<int>*>(r);
                    //                char* str = strdup(rr->toString().toLatin1());
                    char* tag = strdup(rr->tag().toLatin1());
                    PyDict_SetItemString(mod, tag, Py_BuildValue("i",rr->value()));
                    //                free(str);
                    free(tag);
                }

            }
        }

        PyRun_SimpleString("import checkout_plugins as checkout");
        qDebug() << "imported checkout";

        qDebug() << "Python Plugin Running Code";

        FILE* file = _Py_fopen(_script_path.toLatin1(), "r+");
        if (!file)      return;
        PyRun_SimpleFileEx(file, _script_path.toLatin1(), true);
        PyRun_SimpleString("sys.stdout.flush()\nsys.stderr.flush()\n");

        qDebug() << "Python Plugin Post Run" << _results.size();

        // Perform some cleanup & prepare the output data in the "_results set"

        mod = PyModule_GetDict(checkout_instance);
        foreach (RegistrableParent* r, _results.values())
        {
            qDebug() << "Retreiving" << r->tag() << r->toString();
            PyObject* ob = PyDict_GetItemString(mod, r->tag().toLatin1());
            qDebug() <<ob;
            if (PyLong_Check(ob))
            {
                typedef int tt;
                Registrable<tt>* rr = (Registrable<tt>*)(r);
                rr->setValuePointer(new tt());
                rr->setValue((tt)PyLong_AsLong(ob));
            }
            if (PyFloat_Check(ob))
            {
                typedef double tt;
                Registrable<tt>* rr = (Registrable<tt>*)(r);
                rr->setValuePointer(new tt());
                rr->setValue((tt)PyFloat_AsDouble(ob));
            }
            // Check for the

            if (PyArray_Check(ob))
            {

                qDebug() << PyArray_NDIM((PyArrayObject*)ob);

                qDebug() << PyArray_DIM((PyArrayObject*)ob, 0) << PyArray_DIM((PyArrayObject*)ob, 1);

                if (PyArray_NDIM((PyArrayObject*)ob) != 2)
                {
                    PyErr_SetString(CheckoutErrors_plugin, "Array should be a 2D numpy array");
                    //                return NULL;
                }

                int dim0 = PyArray_DIM((PyArrayObject*)ob, 0);
                int dim1 = PyArray_DIM((PyArrayObject*)ob, 1);
                if (PyArray_TYPE((PyArrayObject*)ob) != NPY_UINT16)
                {
                    PyErr_SetString(CheckoutErrors_plugin, "Array should be an array of int16");
                    //                return NULL;
                    // double data = PyGetDouble(pynumpy, i, j);

                    for (int j = 0; j < dim1; ++j)
                        for (int i = 0; i < dim0; ++i)
                        {

                            short data = PyGetType(ob, short, i, j);
                        }


                }
            }



        }

        //    Py_EndInterpreter( pThreadState ) ;
        //    PyEval_ReleaseLock() ; // nb: release the GIL

        qDebug() << "Python Plugin exec() Done";

        //    Py_EndInterpreter(thread_state) ;
        ////    PyEval_RestoreThread(saved);
        //    PyGILState_Release(gstate);
    }

    int getState() { return initState ;}
protected:
    QString _script_path; // Path to the underlying processing script

    int initState; // State of the initalisation:
    bool cloned;
    // 0: Nothing was done
    // Mask by 1: Description was set
    // Mask by 2: at least one "use" function was called
    // Mask by 4: at last a "produce" function was called

};

/*
    checkout.description('Basic/Python/KMEANS', ['wiest.daessle@gmail.com'],
                     'Python script performing kmeans processing');
*/

extern "C" PyObject* checkout_description(PyObject* self, PyObject*args)
{
    char *proc_path, *proc_desc;
    PyObject* author_list;

    if (!PyArg_ParseTuple(args, "sOs", &proc_path, &author_list, &proc_desc))
    {
        PyErr_SetString(CheckoutErrors_plugin, "Error parsing arguments");
        return NULL;
    }
    QStringList authors = pyObjToStringList(author_list);

    // Now set the process properly

    if (current_Process)
        current_Process->description(QString("Python/")+proc_path, authors, proc_desc);

    qDebug() << proc_path << authors << proc_desc;

    Py_INCREF(Py_None);
    return Py_None;
}

/*
 * checkout.use('image', Checkout.Type.Image2D, description='Input Image',
             vector_image=True, properties=['None'])

checkout.use('iterations', Checkout.Type.Int, description='maximum number of iterations',
             default=10, aggregation='Max', range=[0, 100], isSlider=True)

 */

extern "C" PyObject* checkout_use(PyObject* self, PyObject*args, PyObject *keywds)
{
    if (!current_Process) {  Py_INCREF(Py_None); return Py_None; }

    const char* tag="";
    PyObject* type;
    const char* description = "";
    PyObject* vector_img = 0x0;
    PyObject* props = 0x0;
    double def = 0;
    int aggregation = 0;
    PyObject* range = 0x0;
    PyObject* isslider = 0x0;

    static const char *kwlist[] = { "tag",
                                    "type",
                                    "description",
                                    "vector_image",
                                    "properties",
                                    "default",
                                    "aggregation",
                                    "range",
                                    "isSlider",
                                    NULL};

    PyArg_ParseTupleAndKeywords(args, keywds, "sO|sOdOiO", (char**)kwlist,
                                &tag, &type, &description, &vector_img, &props,
                                &def, &aggregation, &range, &isslider);
#define SetIfUseImageType(PYTYPE, CPPTYPE) \
    if (type == PYTYPE) \
    { \
    typedef CPPTYPE tt; \
    tt* i = new tt(); \
    Registrable<tt>& r = current_Process->use(i, tag, description); \
    r.noImageAutoLoading(); \
    if (vector_img == Py_True) r.channelsAsVector(); \
}

    //   current_Process->use(new int(), tag, description).setDefault(1)

#define SetIfUseBasicType(PYTYPE, CPPTYPE) \
    if (type == PYTYPE) \
    { \
    typedef CPPTYPE tt; \
    tt* i = new tt(); \
    Registrable<tt>& r = current_Process->use(i, tag, description); \
    r.setDefault(def); \
}

    SetIfUseImageType(py_Image2D,            cv::Mat             );
    SetIfUseImageType(py_TimeImage,          TimeImage           );
    SetIfUseImageType(py_StackedImage,       StackedImage        );
    SetIfUseImageType(py_TimeStackedImage,   TimeStackedImage    );
    SetIfUseImageType(py_ImageXP,            ImageXP             );
    SetIfUseImageType(py_TimeImageXP,        TimeImageXP         );
    SetIfUseImageType(py_StackedImageXP,     StackedImageXP      );
    SetIfUseImageType(py_TimeStackedImageXP, TimeStackedImageXP  );

    SetIfUseBasicType(py_Int, int);
    SetIfUseBasicType(py_Double, double);

    Py_INCREF(Py_None);
    return Py_None;
}
/*

checkout.use_enum('Algorithm', ['Version1', 'Version 2', '...'], description='',
                  default='Version1')
*/

extern "C" PyObject* checkout_use_enum(PyObject* self, PyObject*args, PyObject *keywds)
{
    if (!current_Process) {  Py_INCREF(Py_None); return Py_None; }

    const char* tag="";
    PyObject* list = 0x0;
    const char* description = "";
    const char* def = "";


    static const char *kwlist[] = { "tag",
                                    "values",
                                    "description",
                                    "default",
                                    NULL};

    PyArg_ParseTupleAndKeywords(args, keywds, "sO|ss", (char**)kwlist,
                                &tag, &list, &description, &def);


    if (PyList_Check(list))
    {

        QStringList data;

        int d = PyList_Size(list);

        for (int i = 0; i < d; ++i)
        {
            PyObject* o = PyList_GetItem(list, i);

            if (PyUnicode_Check(o))
                data << PyUnicode_AsUTF8(o);
        }
        //        qDebug() << data << tag << def;

        RegistrableEnum<int>& r = current_Process->use(new int(), data, tag, description);

        if (QLatin1String("") != def)
        {
            int p = 0;
            for (int i = 0; i < data.size(); ++i)
                if (data.at(i) == def)
                    r.setDefault(p);
        }
    }
    Py_INCREF(Py_None);
    return Py_None;
}

extern "C" PyObject* checkout_produce(PyObject* self, PyObject*args)
{

    if (current_Process)
    {
        char *tag, *description;
        PyObject* type;

        if (!PyArg_ParseTuple(args, "sOs", &tag, &type, &description))
        {
            PyErr_SetString(CheckoutErrors_plugin, "Error parsing arguments");
            return NULL;
        }

//        qDebug() << "Produce" << tag << description;

#define ProdType(PYTYPE, CPPTYPE ) \
    if (type == PYTYPE)\
        {\
    typedef CPPTYPE tt;\
    tt* i = new tt();\
    Registrable<tt>& r = current_Process->produces(i, tag, description);\
    r.noImageAutoLoading();\
    }

#define ProdBasicType(PYTYPE, CPPTYPE ) \
    if (type == PYTYPE)\
        {\
    typedef CPPTYPE tt;\
    tt* i = new tt();\
    Registrable<tt>& r = current_Process->produces(i, tag, description);\
    }

        ProdType(py_Image2D,            cv::Mat             );
        ProdType(py_TimeImage,          TimeImage           );
        ProdType(py_StackedImage,       StackedImage        );
        ProdType(py_TimeStackedImage,   TimeStackedImage    );
        ProdType(py_ImageXP,            ImageXP             );
        ProdType(py_TimeImageXP,        TimeImageXP         );
        ProdType(py_StackedImageXP,     StackedImageXP      );
        ProdType(py_TimeStackedImageXP, TimeStackedImageXP  );

        ProdBasicType(py_Int, int);
        ProdBasicType(py_Double, double);

    }
    Py_INCREF(Py_None);
    return Py_None;
}


static PyMethodDef checkout_methods_plugin[] =
{

    {"description",  checkout_description, METH_VARARGS,
     "description(process_path_string, ['user_string'], 'processing description') -> set the plugin description to be transmited to the user space"},

    {"use",  (PyCFunction)checkout_use, METH_VARARGS | METH_KEYWORDS,
     "use('image', Checkout.Image2D, tag='inputImage', description='Input Image',  vector_image=True, properties=['None'])"
     "-> specify the data to be used by this processing"},

    {"use_enum",  (PyCFunction)checkout_use_enum, METH_VARARGS | METH_KEYWORDS,
     "use_enum -> specify data as an list of enumerated value to be use by this processing"},

    {"produce", checkout_produce, METH_VARARGS,
     "produce('tag', checkout.Int, 'description', value) -> allows to tell the script to output some values"  },



    {NULL, NULL, 0, NULL} /* SENTINEL*/
};

static struct PyModuleDef checkoutmodule_plugin = {

    PyModuleDef_HEAD_INIT,
    "checkout_plugins",   /* name of module */
    "Checkout plugin module, use this to access internal information of the checkout software exposed in the interface", /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
                  or -1 if the module keeps state in global variables. */

    checkout_methods_plugin
};

PyMODINIT_FUNC PyInit_checkoutplugin(void)
{

    PyObject *m;

    m = PyModule_Create(&checkoutmodule_plugin);

    if (m == NULL)
        return NULL;

    CheckoutErrors_plugin = PyErr_NewException("checkout.error", NULL, NULL);
    Py_INCREF(CheckoutErrors_plugin);
    PyModule_AddObject(m, "error", CheckoutErrors_plugin);


    return m;
}


/*
 Expose the following class
    checkout = Checkout()
    checkout.description('Basic/Python/KMEANS', ['wiest.daessle@gmail.com'],
                         'Python script performing kmeans processing');

    checkout.use('image', Checkout.Type.Image2D, tag='inputImage', description='Input Image',
                 vector_image=True, properties=['None'])

    checkout.use('iterations', Checkout.Type.Int, tag='max_iterations', description='maximum number of iterations',
                 default=10, aggregation='Max', range=[0, 100], isSlider=True)

    checkout.use_enum('Algorithm', ['Version1', 'Version 2', '...'], tag='', description='',
                      default='Version1', level='Debug', )
*/




void* import_theimporterd()
{
  import_array();
  return 0x0;
}
CheckoutPythonPluginsHandler::CheckoutPythonPluginsHandler(): _fileWatcher(0), _directoryWatcher(0)
{
    QSettings sets;
    QStringList paths = sets.value("Python/PluginPath", QStringList() << qApp->applicationDirPath() + "/Python/").toStringList();

    QStringList pythonPath = sets.value("Python/PythonPath",
                                        QStringList() <<  QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first() + "\\Anaconda3\\Lib"
                                        <<  QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first() + "\\Anaconda3\\DLLs"
                                        <<  QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first() + "\\Anaconda3\\Lib\\site-packages"
                                        ).toStringList();
    putenv(QString("PYTHONPATH=%1;%2").arg(pythonPath.join(';')).arg(paths.join(';')).toLatin1());

    QString pname("CheckoutPlugin");
    wchar_t p[200]; pname.toWCharArray((wchar_t*)&p);

    PyImport_AppendInittab("checkout_plugins", PyInit_checkoutplugin);

    Py_SetProgramName(p);

    Py_Initialize();

    // Create GIL/enable Threads
    //    PyEval_InitThreads();

    //  thread_state = PyEval_SaveThread() ;

    //  thread_state = PyThreadState_Get();
    //  PyEval_AcquireLock() ; // nb: get the GIL
    //  PyThreadState* pThreadState = Py_NewInterpreter() ;
    //  assert( pThreadState != NULL ) ;
    //  PyEval_AcquireThread( pThreadState ) ;

    checkout_instance = PyImport_ImportModule("checkout_plugins");
    import_theimporterd();

    Py_INCREF(checkout_instance);

    PyRun_SimpleString("import time");
    PyRun_SimpleString("import sys");
    PyRun_SimpleString(QString("sys.stdout = sys.stderr = open(\"%1\", \"w\")").arg(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first() + "/Checkout_PluginPython_Log.txt").toLatin1());

    if (checkout_instance != NULL)
        //  PyModule_AddStringConstant((PyObject*)&checkoutmodule_plugin, "state", "setup");
    {
        PyObject* mod = PyModule_GetDict(checkout_instance);
        PyDict_SetItemString(mod, "state", Py_BuildValue("s", "setup"));

        if (!py_Image2D)
        {
            py_Image2D              = Py_BuildValue("i", 1);
            py_TimeImage            = Py_BuildValue("i", 2);
            py_StackedImage         = Py_BuildValue("i", 3);
            py_TimeStackedImage     = Py_BuildValue("i", 4);
            py_ImageXP              = Py_BuildValue("i", 5);
            py_TimeImageXP          = Py_BuildValue("i", 6);
            py_StackedImageXP       = Py_BuildValue("i", 7);
            py_TimeStackedImageXP   = Py_BuildValue("i", 8);


            py_Int                  = Py_BuildValue("i", 9);
            py_Double               = Py_BuildValue("i", 10);



            Py_INCREF(py_Image2D);
            Py_INCREF(py_TimeImage);
            Py_INCREF(py_StackedImage);
            Py_INCREF(py_TimeStackedImage);
            Py_INCREF(py_ImageXP);
            Py_INCREF(py_TimeImageXP);
            Py_INCREF(py_StackedImageXP);
            Py_INCREF(py_TimeStackedImageXP);

            Py_INCREF(py_Int);
            Py_INCREF(py_Double);


            imageTypes = PyDict_New();

            PyDict_SetItemString(mod, "Image2D", py_Image2D);
            PyDict_SetItemString(mod, "TimeImage", py_TimeImage);
            PyDict_SetItemString(mod, "StackedImage", py_StackedImage);
            PyDict_SetItemString(mod, "TimeStackedImage", py_TimeStackedImage);
            PyDict_SetItemString(mod, "ImageXP", py_ImageXP);

            PyDict_SetItemString(mod, "TimeImageXP", py_TimeImageXP);

            PyDict_SetItemString(mod, "StackedImageXP", py_StackedImageXP);
            PyDict_SetItemString(mod, "TimeStackedImageXP", py_TimeStackedImageXP);

            PyDict_SetItemString(mod, "Int", py_Int);
            PyDict_SetItemString(mod, "Double", py_Double);
            PyDict_SetItemString(mod, "Float", py_Double);
        }
    }
    PyRun_SimpleString("import checkout_plugins as checkout");

    monitorPythonPluginPath(paths);
    //  Py_EndInterpreter( pThreadState ) ;
    //  PyEval_ReleaseLock() ;

}

CheckoutPythonPluginsHandler::~CheckoutPythonPluginsHandler()
{
    Py_DecRef(checkout_instance);
    //    PyEval_RestoreThread( thread_state );
    Py_Finalize();
}


void CheckoutPythonPluginsHandler::monitorPythonPluginPath(QStringList paths)
{

    //  PyRun_SimpleString("print(checkout.state)");
    //PyRun_SimpleString("sys.stdout.flush()\nsys.stderr.flush()\n");


    foreach (QString path, paths)
    {
        QDir dir(path);
        if (!dir.exists())
        {
            dir.mkpath(path);
            dir.cd(path);
        }

        qDebug() << "Monitoring directory: " << path;
        QStringList pfiles = dir.entryList(QStringList() << "*.py"/*, QDir::Files*/);
        qDebug() << "Monitoring files: " << pfiles;
        QStringList files;

        foreach (QString f, pfiles)
        {
            QString ff = path + '/' + f;

            // Check if ff is a compatible python file...
            // if yes add to monitoring, else leave away

            FILE* file = 0;

            file = _Py_fopen(ff.toLatin1(), "r+");
            if (!file)
                return;

            current_Process = _scripts[ff];
            if (!current_Process)
            {
                current_Process = new PythonProcess(ff);
                _scripts[ff] = current_Process;
            }


            PyRun_SimpleFileEx(file, ff.toLatin1(), true);
            //          PyRun_FileEx(file, filename.toLatin1(),
            //                       0, );
            PyRun_SimpleString("sys.stdout.flush()\nsys.stderr.flush()\n");
            if (current_Process->getState() != 0)
            {
                CheckoutProcess::handler().addProcess(current_Process);

                files << ff;

            }
        }

        if (!_fileWatcher && files.size() != 0)
        {
            _fileWatcher = new QFileSystemWatcher(files );
            connect(_fileWatcher, SIGNAL(fileChanged(QString)), this, SLOT(fileChanged(QString)));
        }
        else
            if (files.size())
                _fileWatcher->addPaths(files);

        if (!_directoryWatcher)
        {
            _directoryWatcher = new QFileSystemWatcher(QStringList() << path);
            connect(_directoryWatcher, SIGNAL(directoryChanged(QString)), this, SLOT(directoryChanged(QString)));
        }
        _directoryWatcher->addPath(path);
    }
}

void CheckoutPythonPluginsHandler::directoryChanged(const QString &path)
{
    //  qDebug() << "Python Plugins, directory changed in path: "  << path;

    //  QStringList ls = _fileWatcher->files();
    //  ls.contains(f)

    // Look for new files && removed ones
    //  _fileWatcher->files()
    QDir dir(path);
    QStringList pfiles = dir.entryList(QStringList() << "*.py"/*, QDir::Files*/);
    QStringList files = _fileWatcher->files();
    QStringList final_files;
    foreach (QString p, pfiles)
    {
        if (!files.contains(p))
        {

            FILE* file = 0;

            file = _Py_fopen(p.toLatin1(), "r+");
            if (!file)
                return;

            current_Process = _scripts[p];
            if (!current_Process)
            {
                current_Process = new PythonProcess(p);
                _scripts[p] = current_Process;
            }


            PyRun_SimpleFileEx(file, p.toLatin1(), true);
            //          PyRun_FileEx(file, filename.toLatin1(),
            //                       0, );
            PyRun_SimpleString("sys.stdout.flush()\nsys.stderr.flush()\n");
            if (current_Process->getState() != 0)
            {
                CheckoutProcess::handler().addProcess(current_Process);
                final_files << p;
            }


        }
    }
    if (final_files.size())
        _fileWatcher->addPaths(final_files);


}

void CheckoutPythonPluginsHandler::fileChanged(const QString &path)
{
    qDebug() << "Python Plugins, file changed in path: "  << path;

    // File has changed find corresponding plugin & update plugin's information
    // Load file & exec the proper function...
    QString ff = path;

    current_Process = _scripts[ff];

    if (current_Process)
    {
        CheckoutProcess::handler().removeProcess(current_Process);
        _scripts.remove(ff);
    }

    FILE* file = 0;

    file = _Py_fopen(ff.toLatin1(), "r+");
    if (!file)
        return;

    current_Process = _scripts[ff];
    current_Process = new PythonProcess(ff);
    _scripts[ff] = current_Process;


    PyRun_SimpleFileEx(file, ff.toLatin1(), true);
    //          PyRun_FileEx(file, filename.toLatin1(),
    //                       0, );
    PyRun_SimpleString("sys.stdout.flush()\nsys.stderr.flush()\n");
    if (current_Process->getState() != 0)
        CheckoutProcess::handler().addProcess(current_Process);
}
