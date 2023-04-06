#include "Python.h"
#include "pyerrors.h"
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include <numpy/arrayobject.h>



#include "checkoutcorepythoninterface.h"

#include <QString>
#include <QStandardPaths>
#include <Core/wellplatemodel.h>
#include <Core/checkoutdataloaderplugininterface.h>

#include <cstdlib>

/*
class Checkout:
    """ --- Checkout core exposed data"""
    def loadedWellPlates(self):
        return ["plate 1", "Plate 2"]

    def availableDatasets(self):
        return dict({'wellplate name': { "data 1 ", "data 2" }})


    def getDataset(self, platename, dataname):
        pass

    def addDataset(self, platename, dataname, array):
        pass

*/
// extern "C"  PyObject* checkout_loadedWellPlates(PyObject* self, PyObject*args);
static PyObject *CheckoutErrors;
static CheckoutCorePythonInterface* py_WinAccess = 0;

extern "C" PyObject* checkout_loadedWellPlates(PyObject* self, PyObject*args)
{

  Screens& scr = ScreensHandler::getHandler().getScreens();


  PyObject* list = PyList_New(0);
  foreach (ExperimentFileModel* mdl, scr)
    {
      char* name = strdup(mdl->name().toLatin1());
      PyObject* val = Py_BuildValue("s", name);
      PyList_Append(list, val);
      free(name);
    }

  return list;
}


extern "C" PyObject* checkout_getSelectedWellPlates(PyObject* self, PyObject* args)
{
  PyObject* list = PyList_New(0);

  foreach (QString sname, py_WinAccess->getSelectedWellplates())
    {
      char* name = strdup(sname.toLatin1());
      PyObject* val = Py_BuildValue("s", name);
      PyList_Append(list, val);
      free(name);
    }



  return list;
}


//def availableDatasets(self):
//    return dict({'wellplate name': { "data 1 ", "data 2" }})

extern "C"  PyObject* checkout_availableDatasets(PyObject* self, PyObject*args)
{

  PyObject* res = PyDict_New();

  Screens scr =ScreensHandler::getHandler().getScreens();
  foreach (ExperimentFileModel* mdl, scr)
    {
      PyObject* list = PyList_New(0);

      QStringList data=mdl->computedDataModel()->getExperiments();
      //      qDebug() << mdl->computedDataModel()->columnCount() << data;
      foreach(QString ds, data)
        {

          char* name = strdup(ds.toLatin1());
          PyObject* val = Py_BuildValue("s", name);
          PyList_Append(list, val);
          //          Py_DECREF(val);
          free(name);
        }

    }

  //  Py_DECREF(res);
  return res;
}


// print(checkout.getDataset(['AssayPlate_Greiner_#781956'],['m_Basic_Otsu_Threshold_Threshold_Isolated_Area_Count']))
// print(checkout.getDataset(['AssayPlate_Greiner_#781956'],'m_Basic_Otsu_Threshold_Threshold_Isolated_Area_Count'))

QStringList pyObjToStringList(PyObject* ob)
{
  QStringList res;

  if (PyUnicode_Check(ob))
    {
      Py_ssize_t size;
      wchar_t* str = PyUnicode_AsWideCharString(ob, &size);
      res <<  QString::fromWCharArray(str,size);
      PyMem_Free(str);
    }
  else
    if (PySequence_Check(ob))
      {
        for (int i = 0; i < PySequence_Size(ob); ++i)
          {
            PyObject* itm = PySequence_GetItem(ob, i);
            if (!PyUnicode_Check(itm)) continue;
            Py_ssize_t size;
            wchar_t* str = PyUnicode_AsWideCharString(itm, &size);
            res <<  QString::fromWCharArray(str,size);
            PyMem_Free(str);
          }

      }

  return res;
}


//def getDataset(self, platename, dataname):
extern "C"  PyObject* checkout_getDataset(PyObject* self, PyObject*args)
{

  PyObject *lplatename, *ldataname;

  //  const char* platename, dataname;

  //  PyList_Type

  if (!PyArg_ParseTuple(args, "OO", & lplatename, &ldataname))
    {
      PyErr_SetString(CheckoutErrors, "Error parsing arguments");

      return NULL;
    }

  QStringList plates = pyObjToStringList(lplatename);
  if (plates.isEmpty())
    {
      PyErr_SetString(CheckoutErrors, "Empty platename sequence");
      return NULL;
    }

  QStringList xps = pyObjToStringList(ldataname);
  if (xps.isEmpty())
    {
      PyErr_SetString(CheckoutErrors, "Empty dataset name sequence");
      return NULL;
    }


  QList<MatrixDataModel*> mdls;
  int cols = 0;
  Screens scr =ScreensHandler::getHandler().getScreens();
  foreach (ExperimentFileModel* mdl, scr)
    if (plates.contains(mdl->name()))
      {
        //  QStringList data=mdl->computedDataModel()->getExperiments();
        QStringList memexp=mdl->computedDataModel()->getExperiments();
        QStringList memdata, dbdata;
        foreach (QString t, xps) (memexp.contains(t) ? memdata : dbdata) << t;

        QVector<QList<int> > subdata(4);

        mdls << mdl->computedDataModel()->exposeData(memdata, dbdata, subdata);
        cols += mdls.last()->columnCount();
      }

  if (!cols)
    {
      Py_INCREF(Py_None);
      return  Py_None;
    }

  //  qDebug() << mdls.size();
  // Allocate memory with python for coherency, the ownership of the data is transfered to python
  double* rawdata = (double*)PyMem_MALLOC(cols*27*27*sizeof(double));
  double* p = rawdata;

  PyObject* header = PyList_New(0);
  PyObject* rows = PyList_New(0);
  PyObject* arows = PyList_New(0);
  {
    MatrixDataModel* mdl = mdls.first();
    for (int i = 0; i < mdl->rowCount(); ++i)
      {
        QString l = mdl->rowName(i);
        wchar_t str[400];
        l.toWCharArray((wchar_t*)&str);
        PyList_Append(rows, PyUnicode_FromWideChar(str, l.size()));
        PyList_Append(arows, PyBool_FromLong(mdl->isActive(i)));
      }
  }


  foreach (MatrixDataModel* mdl, mdls)
    {
      QVector<QVector<double> >& data = (*mdl)();
      for (int i = 0; i < data.size(); ++i)
        {
          QString l  = mdl->columnName(i);
          wchar_t str[400];
          l.toWCharArray((wchar_t*)&str);
          PyList_Append(header, PyUnicode_FromWideChar(str, l.size()));

          for (int j = 0; j < data[i].size(); ++j,++p)
            *p = data[i][j];
        }
      delete mdl;
    }
  mdls.clear();

  // Set up the dataset
  npy_intp ar[] = { 2, 27*27};
  //  static double dataarr[] = { 1, 1, 1,
  //                              2, 2, 2};
  int nd=2;
  ar[0] = cols;
  // Create a numpy array, and transfer ownership of data to python
  PyObject* r = PyArray_New(&PyArray_Type, nd, (npy_intp*)&ar, NPY_DOUBLE, NULL, \
                            rawdata, 0, NPY_ARRAY_OWNDATA|NPY_ARRAY_CARRAY, NULL);
  // And now build the NumpyArray based on all the mdls data :)

  PyObject* res = PyDict_New();

  PyDict_SetItemString(res, "Rows", rows);
  PyDict_SetItemString(res, "activeRows", arows);
  PyDict_SetItemString(res, "Header", header);
  PyDict_SetItemString(res, "Data", r);

  return res;
}

#define PyGetDouble(obj, i, j) *(double *)PyArray_GETPTR2((PyArrayObject*)(obj), (i), (j))

//def addDataset(self, platename, dataname, array):
extern "C"  PyObject* checkout_addDataset(PyObject* self, PyObject*args)
{

  PyObject *lplatename, *ldataname,*pynumpy;

  //  const char* platename, dataname;
  //  PyList_Type

  if (!PyArg_ParseTuple(args, "OOO", & lplatename, &ldataname, &pynumpy))
    {
      PyErr_SetString(CheckoutErrors, "Parameter list error, provide a string for platename, a sequence of string for dataname and a numpyarray for dataset");
      return NULL;
    }

  QStringList plates = pyObjToStringList(lplatename);
  if (plates.isEmpty() || plates.size() > 1)
    {
      PyErr_SetString(CheckoutErrors, "Set one plate name for the storage");
      return NULL;
    }

  QStringList xps = pyObjToStringList(ldataname);
  if (xps.isEmpty())
    {
      return NULL;
    }
  if (!PyArray_Check(pynumpy))
    {
      PyErr_SetString(CheckoutErrors, "Array should be a numpy object");

      return NULL;
    }
  if (PyArray_NDIM((PyArrayObject*)pynumpy) != 2)
    {
      PyErr_SetString(CheckoutErrors, "Array should be a 2D numpy array");
      return NULL;
    }

  int dim0 = PyArray_DIM((PyArrayObject*)pynumpy, 0);
  int dim1 = PyArray_DIM((PyArrayObject*)pynumpy, 1);

  if (dim0  != P2('Z'-'A'+1))
    {
      PyErr_SetString(CheckoutErrors, "Expecting first array dimension to be 29");
      return NULL;
    }

  if (dim1 != xps.size())
    {
      PyErr_SetString(CheckoutErrors, "Expecting second array dimension to be the same as the number of text fields");
      return NULL;
    }

  if (PyArray_TYPE((PyArrayObject*)pynumpy) != NPY_DOUBLE)
    {
      PyErr_SetString(CheckoutErrors, "Array should be an array of double");
      return NULL;
    }

  QString plate =  plates.first();
  Screens scr =ScreensHandler::getHandler().getScreens();
  foreach (ExperimentFileModel* mdl, scr)
    if (plate == mdl->name())
      {
        ExperimentDataModel *xpmdl = mdl->computedDataModel();

        //        xpmdl->hash();

        MatrixDataModel m(0);
        int col = 0;
        foreach (QString xp, xps)
          {

            for (int j = 0; j < dim1; ++j)
              for (int i = 0; i < dim0; ++i)
                {

                  QPoint p = m.rowPos(i);

                  if (mdl->hasMeasurements(p))
                    {
                      double data = PyGetDouble(pynumpy, i, j);
                      xpmdl->addData(xp, 1, 1, 1, 1, p.x(), p.y(), data);
                    }

                }

            col++;
          }
      }


  Py_INCREF(Py_None);
  return Py_None;
}

//def "commit"Dataset(self, platename, dataname, colname):
/// FIXME: Finish the data commit function
extern "C"  PyObject* checkout_commitDataset(PyObject* self, PyObject*args)
{
  Py_INCREF(Py_None);
  return Py_None;
}


//# Add this interfaces
//    def dataPath(self):
//        return ["c:/bob", "g:/data"]
extern "C"  PyObject* checkout_dataPath(PyObject* self, PyObject*args)
{
  QSettings set;
  QStringList l = set.value("ScreensDirectory", QVariant(QStringList())).toStringList();

  PyObject* list = PyList_New(0);
  foreach (QString s, l)
    {
      char* name = strdup(s.toLatin1());
      PyObject* val = Py_BuildValue("s", name);
      PyList_Append(list, val);
      free(name);
    }

  return list;

  //  Py_INCREF(Py_None);
  //  return Py_None;
}


QStringList recurse(QString dir, QStringList search, QStringList& res)
{

  for (QStringList::iterator it = search.begin(),e = search.end();
       it != e; ++it)
    {
      QFileInfo i(dir + "/" + *it);
      if (i.exists())
        {
          //          qDebug() << "Adding" << i.fileName() << i.filePath() << i.absolutePath();
          res << i.filePath();
        }
    }

  QDir ddir(dir);

  QStringList ents = ddir.entryList(QStringList() << "*", QDir::AllDirs | QDir::NoDotAndDotDot);
  for (QStringList::Iterator it = ents.begin(), e = ents.end(); it != e; ++it)
    {
      recurse(dir + "/" + *it, search, res);
    }

  return res;
}

//    def loadableWellPlates(self):
//        return ["c:/bob/plate/data.mrf", "g:/data/plate1/data.mrf", "g:/data/plate2/data2.mrf"]
extern "C"  PyObject* checkout_loadableWellPlates(PyObject* self, PyObject*args)
{

  QSettings set;
  QStringList l = set.value("ScreensDirectory", QVariant(QStringList())).toStringList();

  QStringList search = CheckoutDataLoader::handler().handledFiles();


  PyObject* list = PyList_New(0);
  foreach (QString s, l)
    {
      QStringList res;
      recurse(s, search, res);
      foreach (QString f, res)
        {
          char* name = strdup(f.toLatin1());
          PyObject* val = Py_BuildValue("s", name);
          PyList_Append(list, val);
          free(name);
        }
    }

  return list;
}

//    def addPath(self, dir):
//        pass
extern "C"  PyObject* checkout_addPath(PyObject* self, PyObject*args)
{
  PyObject *dirs;
  if (!PyArg_ParseTuple(args, "O", & dirs))
    {
      PyErr_SetString(CheckoutErrors, "Error parsing arguments");
      return NULL;
    }

  QStringList d = pyObjToStringList(dirs);

  foreach (QString str, d)
    {
      QFileInfo f(str);
      if (f.isDir() && py_WinAccess)
        py_WinAccess->co_addDirectory(str);
      else
        PyErr_SetString(CheckoutErrors, "Error adding path");
    }

  Py_INCREF(Py_None);
  return Py_None;
}

//    def deletePath(self, dir):
//        pass
extern "C"  PyObject* checkout_deletePath(PyObject* self, PyObject*args)
{
  PyObject *dirs;
  if (!PyArg_ParseTuple(args, "O", & dirs))
    {
      PyErr_SetString(CheckoutErrors, "Error parsing arguments");
      return NULL;
    }

  QStringList d = pyObjToStringList(dirs);

  foreach (QString str, d)
    {
      //      if (!search.contains(str))
      //        {
      ////          PyErr_SetString(CheckoutErrors, "Error f");
      //        }
      QFileInfo f(str);
      if (f.isDir() && py_WinAccess)
        py_WinAccess->co_deleteDirectory(str);
      else
        PyErr_SetString(CheckoutErrors, "Error removing path");
    }


  Py_INCREF(Py_None);
  return Py_None;
}

//def loadWellPlate(self, name):
//       pass
// checkout.loadWellplate('C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/MeasurementDetail.mrf')
extern "C"  PyObject* checkout_loadWellPlate(PyObject* self, PyObject*args)
{
  PyObject *dirs;
  if (!PyArg_ParseTuple(args, "O", & dirs))
    {
      PyErr_SetString(CheckoutErrors, "Error parsing arguments");
      return NULL;
    }

  QStringList d = pyObjToStringList(dirs);
  if (py_WinAccess)
    py_WinAccess->co_loadWellplates(d);


  Py_INCREF(Py_None);
  return Py_None;
}

void buildSequenceDict(PyObject* dseq, QList<SequenceFileModel*>& lsfm)
{
  foreach (SequenceFileModel* sfm, lsfm)
    {
      PyObject* pos = PyBytes_FromString(strdup(sfm->Pos().toLatin1()));
      PyObject* dField = PyDict_New();
      for (unsigned f = 1; f <= sfm->getFieldCount(); ++f)
        {
          PyObject* dstack = PyDict_New();
          PyObject* fie =  Py_BuildValue("i", f);
          for (unsigned z = 1; z <= sfm->getZCount(); ++z)
            {
              PyObject* dtime = PyDict_New();
              PyObject* stackZ = Py_BuildValue("i", z);

              for (unsigned t = 1; t <= sfm->getTimePointCount(); ++t)
                {
                  PyObject* dchans = PyDict_New();
                  PyObject* time = Py_BuildValue("i", t);
                  foreach (unsigned c, sfm->getChannelsIds())
                    {
                      PyObject* chan = Py_BuildValue("i", c);
                      PyObject* file = PyBytes_FromString(strdup(sfm->getFileChanId(t,f,z,c).toLatin1()));
                      PyDict_SetItem(dchans, chan, file);
                    }
                  if (PyDict_Size(dchans) != 0)
                    {
                      PyDict_SetItem(dtime, stackZ, dchans);
                    }
                }
              if (PyDict_Size(dtime) != 0)
                {
                  PyDict_SetItem(dstack, stackZ, dtime);
                }
            }
          if (PyDict_Size(dstack) != 0)
            {
              PyDict_SetItem(dField, fie, dstack);
            }
        }
      if (PyDict_Size(dField) != 0)
        {
          PyDict_SetItem(dseq, pos, dField);
        }
    }

}


extern "C" PyObject* checkout_showImageFile(PyObject* self, PyObject* args)
{
  PyObject *dirs;
  if (!PyArg_ParseTuple(args, "O", & dirs))
    {
      PyErr_SetString(CheckoutErrors, "Error parsing arguments");
      return NULL;
    }

  QStringList d = pyObjToStringList(dirs);

  if (py_WinAccess)
    py_WinAccess->co_addImages(d);

  Py_INCREF(Py_None);
  return Py_None;
}



//def getSelectedImageFileNames(self, name):
//       pass
// d=checkout.getSelectedImageFileNames()
extern "C"  PyObject* checkout_getSelectedImageFileNames(PyObject* self, PyObject*args)
{
  PyObject* res = PyDict_New();

  // Walk down the path of Sequence / Expe / WellPlate / Field / Stack / Time / Channel / Filename

  ScreensHandler& handler = ScreensHandler::getHandler();
  Screens& scr = handler.getScreens();
  foreach (ExperimentFileModel* xpl, scr)
    {
      PyObject* seq = PyBytes_FromString(strdup(xpl->name().toLatin1()));
      QList<SequenceFileModel*> lsfm = xpl->getSelection();
      PyObject* dseq = PyDict_New();

      buildSequenceDict(dseq, lsfm);

      if (PyDict_Size(dseq) != 0)
        {
          PyDict_SetItem(res, seq, dseq);
        }
    }


  return res;

  Py_INCREF(Py_None);
  return Py_None;
}

//def getLoadedImageFileNames()(self, name):
//       pass
// d=checkout.getSelectedImageFileNames()
extern "C"  PyObject* checkout_getLoadedImageFileNames(PyObject* self, PyObject*args)
{
  PyObject* res = PyDict_New();

  // Walk down the path of Sequence / Expe / WellPlate / Field / Stack / Time / Channel / Filename

  ScreensHandler& handler = ScreensHandler::getHandler();
  Screens& scr = handler.getScreens();
  foreach (ExperimentFileModel* xpl, scr)
    {
      PyObject* seq = PyBytes_FromString(strdup(xpl->name().toLatin1()));
      QList<SequenceFileModel*> lsfm = xpl->getValidSequenceFiles();
      PyObject* dseq = PyDict_New();

      buildSequenceDict(dseq, lsfm);

      if (PyDict_Size(dseq) != 0)
        {
          PyDict_SetItem(res, seq, dseq);
        }
    }


  return res;

  Py_INCREF(Py_None);
  return Py_None;
}

// checkout.selectWells('platename', [ 'A2', 'A3' ])
extern "C"  PyObject* checkout_selectWells(PyObject* self, PyObject*args)
{
  char* plate;
  PyObject *wells;
  if (!PyArg_ParseTuple(args, "sO", & plate, & wells))
    {
      PyErr_SetString(CheckoutErrors, "Error parsing arguments");
      return NULL;
    }

  QStringList d = pyObjToStringList(wells);

  //    qDebug() << plate << d;
  Screens scr =ScreensHandler::getHandler().getScreens();
  foreach (ExperimentFileModel* mdl, scr)
    if (mdl->name() == plate)
      {
        mdl->clearState(ExperimentFileModel::IsSelected);

        ExperimentDataTableModel* xpmdl = mdl->computedDataModel();
        foreach (QString po, d)
          {
            //                qDebug() << plate << po;
            mdl->select(xpmdl->stringToPos(po), true);
          }
      }

  Py_INCREF(Py_None);
  return Py_None;
}


// checkout.displaySelection()
extern "C"  PyObject* checkout_displaySelection(PyObject* self, PyObject*args)
{

  if ( py_WinAccess)
    py_WinAccess->co_displayWells();
  Py_INCREF(Py_None);
  return Py_None;
}

//print(checkout.loadableWellPlates())
//checkout.loadWellplate(checkout.loadableWellPlates()[0])

static PyMethodDef checkout_methods[] =
{
  {"loadedWellPlates",  checkout_loadedWellPlates, METH_VARARGS,
   "loadedWellPlates() -> returns the loaded well plate names"},

  {"selectedWellPlates", checkout_getSelectedWellPlates, METH_VARARGS,
   "selectedWellPlates() -> returns the names of the selected well plates" },

  {"availableDatasets",  checkout_availableDatasets, METH_VARARGS,
   "availableDatasets() -> returns the names of available dataset"},

  {"getDataset",  checkout_getDataset, METH_VARARGS,
   "getDataset(platename (string sequence or string), fields (string sequence or string)) -> retrieve specified datasets"},

  {"addDataset",  checkout_addDataset, METH_VARARGS,
   "add the specified data to the dataset"},

  //  {"commitDataset",  checkout_commitDataset, METH_VARARGS,
  //   "Commit the specified dataset to the database"},

  {"dataPath",checkout_dataPath, METH_VARARGS,
   "get Data path of loadable wellplates"},

  {"loadableWellPlates",checkout_loadableWellPlates, METH_VARARGS,
   "get list of loadable wellplates"},

  {"addPath",checkout_addPath, METH_VARARGS,
   "Add a path to the loadable wellplate group"},

  {"deletePath",checkout_deletePath, METH_VARARGS,
   "delete a path from the loadable wellplate group"},

  { "loadWellplate", checkout_loadWellPlate, METH_VARARGS,
    "load specified wellplates"},

  { "getSelectedImageFileNames", checkout_getSelectedImageFileNames, METH_VARARGS,
    "returns a dictionnary of the selected image filenames"},

  { "getLoadedImageFileNames", checkout_getLoadedImageFileNames, METH_VARARGS,
    "returns a dictionnary of the loaded image filenames"},

  { "selectWells", checkout_selectWells, METH_VARARGS,
    "select Wells from specified platename selectWells(platename, well_list)"},

  { "displaySelection", checkout_displaySelection, METH_VARARGS,
    "push wells selected with select wells into the display area" },

  { "showImageFile", checkout_showImageFile, METH_VARARGS,
    "showImageFile(['c:/data/im0_C1.tif', 'c:/data/im0_c2.tif']) Add the specified files to the visualisation zone as a multichannel image" },



  {NULL, NULL, 0, NULL} /* SENTINEL*/
};


static struct PyModuleDef checkoutmodule = {

  PyModuleDef_HEAD_INIT,
  "PhenoLink",   /* name of module */
  "PhenoLink module, use this to access internal information of the checkout software exposed in the interface", /* module documentation, may be NULL */
  -1,       /* size of per-interpreter state of the module,
                  or -1 if the module keeps state in global variables. */

  checkout_methods
};

PyMODINIT_FUNC PyInit_checkout(void)
{

  PyObject *m;

  m = PyModule_Create(&checkoutmodule);
  if (m == NULL)
    return NULL;

  CheckoutErrors = PyErr_NewException("checkout.error", NULL, NULL);
  Py_INCREF(CheckoutErrors);
  PyModule_AddObject(m, "error", CheckoutErrors);
  return m;

}


void* import_theimporter()
{
  import_array();
  return 0x0;
}


CheckoutCorePythonInterface::CheckoutCorePythonInterface()
{
  QSettings sets;

  QStringList pythonPath = sets.value("Python/PythonPath",
                                      QStringList() <<  QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first() + "\\Anaconda3\\Lib"
                                      <<   QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first() + "\\Anaconda3\\DLLs"
                                      <<  QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first() + "\\Anaconda3\\Lib\\site-packages"
                                      ).toStringList();

  putenv(QString("PYTHONPATH=%1").arg(pythonPath.join(';')).toLatin1());

  py_WinAccess = this;

  QString pname("PhenoLinkCore");
  wchar_t p[200]; pname.toWCharArray((wchar_t*)&p);

  PyImport_AppendInittab("PhenoLink", PyInit_checkout);

  Py_SetProgramName(p);

  Py_Initialize();
  PyImport_ImportModule("PhenoLink");
  import_theimporter();


  PyRun_SimpleString("import sys");
  PyRun_SimpleString(QString("sys.stdout = sys.stderr = open(\"%1\", \"w\")").arg(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first() + "/PhenoLink_Python_Log.txt").toLatin1());
  PyRun_SimpleString("import PhenoLink");
  PyRun_SimpleString("__name__='__checkout__'");
  //PyRun_SimpleString("help(checkout)");

  /*
  PyEval_InitThreads();
  PyThreadState* main_state = PyThreadState_Get();
  PyEval_ReleaseLock();

*/
}

CheckoutCorePythonInterface::~CheckoutCorePythonInterface()
{
  Py_DECREF(CheckoutErrors);
  Py_Finalize();
}

void CheckoutCorePythonInterface::runCode(QString code)
{
  //  qDebug() << "Running python code";
  PyRun_SimpleString(code.toLatin1());

  PyRun_SimpleString("sys.stdout.flush()\nsys.stderr.flush()\n");
}

void CheckoutCorePythonInterface::runFile(QString filename)
{
  FILE* file = 0;

  file = _Py_fopen(filename.toLatin1(), "r+");
  if (!file)
    return;

  PyRun_SimpleFileEx(file, filename.toLatin1(), true);

  PyRun_SimpleString("sys.stdout.flush()\nsys.stderr.flush()\n");

}

void CheckoutCorePythonInterface::setSelectedWellPlates(QStringList wellp)
{
  selectedWellplates = wellp;
}

QStringList CheckoutCorePythonInterface::getSelectedWellplates()
{
  return selectedWellplates;
}

void CheckoutCorePythonInterface::co_addDirectory(QString name)
{
  emit addDirectory(name);
}

void CheckoutCorePythonInterface::co_deleteDirectory(QString name)
{
  emit deleteDirectory(name);
}

void CheckoutCorePythonInterface::co_loadWellplates(QStringList plates)
{
  emit loadWellplates(plates);
}

void CheckoutCorePythonInterface::co_displayWells()
{
  emit displayWellSelection();
}

void CheckoutCorePythonInterface::co_addImages(QStringList files)
{
  emit addImage(files);
}
