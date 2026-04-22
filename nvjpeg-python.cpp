#include <stdio.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <Python.h>
#include <pythread.h>
#include <structmember.h>
#include <string.h>
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>
#include "JpegCoder.hpp"

typedef struct
{
    PyObject_HEAD
    JpegCoder* m_handle;
}NvJpeg;


static PyMemberDef NvJpeg_DataMembers[] =
{
        {(char*)"m_handle",   T_OBJECT, offsetof(NvJpeg, m_handle),   0, (char*)"NvJpeg handle ptr"},
        {NULL, 0, 0, 0, NULL}
};

int NvJpeg_init(PyObject *self, PyObject *args, PyObject *kwds) {
  ((NvJpeg*)self)->m_handle = new JpegCoder();
  return 0;
}


static void NvJpeg_Destruct(PyObject* self)
{
    delete (JpegCoder*)(((NvJpeg*)self)->m_handle);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* NvJpeg_Str(PyObject* Self)
{
    return Py_BuildValue("s", "<nvjpeg-python.nvjpeg>");
}

static PyObject* NvJpeg_Repr(PyObject* Self)
{
    return NvJpeg_Str(Self);
}

static PyObject* NvJpeg_decode(NvJpeg* Self, PyObject* Argvs)
{
    JpegCoder* m_handle = (JpegCoder*)Self->m_handle;
    
    Py_buffer pyBuf;
    const char* output = NULL;
    unsigned char* jpegData;
    Py_ssize_t len;
    if(!PyArg_ParseTuple(Argvs, "y*|s", &pyBuf, &output)){
        PyErr_SetString(PyExc_ValueError, "Parse the argument FAILED! You should jpegData byte string!");
        return NULL;
    }
    jpegData = (unsigned char*)pyBuf.buf;
    len = pyBuf.len;
    if (output == NULL) {
        output = getenv("PYNVJPEG_DECODE_OUTPUT");
        if (output == NULL) {
            output = "bgr";
        }
    }

    nvjpegOutputFormat_t output_format = NVJPEG_OUTPUT_BGRI;
    if (strcmp(output, "rgb") == 0) {
        output_format = NVJPEG_OUTPUT_RGBI;
    } else if (strcmp(output, "bgr") != 0) {
        PyBuffer_Release(&pyBuf);
        PyErr_SetString(PyExc_ValueError, "decode output must be 'rgb' or 'bgr'");
        return NULL;
    }

    JpegCoderImage* img;
    try{
        m_handle->ensureThread(PyThread_get_thread_ident());
        img = m_handle->decode((const unsigned char*)jpegData, (size_t)len, output_format);
        PyBuffer_Release(&pyBuf);
    }catch(JpegCoderError e){
        PyBuffer_Release(&pyBuf);
        PyErr_Format(PyExc_ValueError, "%s, Code: %d", e.what(), e.code());
        return NULL;
    }

    unsigned char* data = img->buffer();

    npy_intp dims[3] = {(npy_intp)(img->height), (npy_intp)(img->width), 3};
    PyObject* temp = PyArray_SimpleNewFromData(3, dims, NPY_UINT8, data);

    PyArray_ENABLEFLAGS((PyArrayObject*) temp, NPY_ARRAY_OWNDATA);
    delete(img);
    return temp;
}

static PyObject* NvJpeg_encode_impl(NvJpeg* Self, PyArrayObject *vecin, unsigned int quality, const char* input)
{
    if (NULL == vecin){
        Py_INCREF(Py_None);
        return Py_None;
    }

    if (PyArray_NDIM(vecin) != 3){
        PyErr_SetString(PyExc_ValueError, "Parse the argument FAILED! You should pass BGR image numpy array by height*width*channel!");
        return NULL;
    }

    if(quality>100){
        quality = 100;
    }

    nvjpegInputFormat_t input_format = NVJPEG_INPUT_BGRI;
    if (strcmp(input, "rgb") == 0) {
        input_format = NVJPEG_INPUT_RGBI;
    } else if (strcmp(input, "bgr") != 0) {
        PyErr_SetString(PyExc_ValueError, "encode input must be 'rgb' or 'bgr'");
        return NULL;
    }

    JpegCoder* m_handle = (JpegCoder*)Self->m_handle;

    PyObject* bytes = PyObject_CallMethod((PyObject*)vecin, "tobytes", NULL);
    if (bytes == NULL) {
        return NULL;
    }

    Py_buffer pyBuf;
    if (!PyArg_Parse(bytes, "y*", &pyBuf)) {
        Py_DECREF(bytes);
        return NULL;
    }

    unsigned char* buffer = (unsigned char*)pyBuf.buf;
    auto img = new JpegCoderImage(PyArray_DIM(vecin, 1), PyArray_DIM(vecin, 0), 3, JPEGCODER_CSS_444);
    img->fill(buffer);
    PyBuffer_Release(&pyBuf);
    Py_DECREF(bytes);

    m_handle->ensureThread(PyThread_get_thread_ident());
    auto data = m_handle->encode(img, quality, input_format);

    PyObject* rtn = PyBytes_FromStringAndSize((const char*)data->data, data->size);

    delete(data);
    delete(img);

    return rtn;
}

static PyObject* NvJpeg_encode(NvJpeg* Self, PyObject* Argvs, PyObject* Kwds)
{
    PyArrayObject *vecin;
    unsigned int quality = 70;
    const char* input = "bgr";
    static char *kwlist[] = {(char*)"image", (char*)"quality", (char*)"input", NULL};

    if (!PyArg_ParseTupleAndKeywords(Argvs, Kwds, "O!|Is", kwlist, &PyArray_Type, &vecin, &quality, &input)){
        PyErr_SetString(PyExc_ValueError, "Parse the argument FAILED! You should pass BGR image numpy array!");
        return NULL;
    }
    return NvJpeg_encode_impl(Self, vecin, quality, input);
}

static PyObject* NvJpeg_read(NvJpeg* Self, PyObject* Argvs)
{
    JpegCoder* m_handle = (JpegCoder*)Self->m_handle;
    
    unsigned char* jpegFile;
    if(!PyArg_ParseTuple(Argvs, "s", &jpegFile)){
        PyErr_SetString(PyExc_ValueError, "Parse the argument FAILED! You should pass jpeg file path string!");
        return NULL;
    }
    #ifdef _MSC_VER
    FILE* fp;
    fopen_s(&fp, (const char*)jpegFile, "rb");
    #else
    FILE* fp = fopen((const char*)jpegFile, "rb");
    #endif

    if (fp == NULL){
        PyErr_Format(PyExc_IOError, "Cannot open file \"%s\"", jpegFile);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    size_t dataLength = ftell(fp);
    unsigned char* jpegData = (unsigned char*)malloc(dataLength);
    if(jpegData == NULL){
        fclose(fp);
        PyErr_Format(PyExc_IOError, "Out of memeroy when read file \"%s\"", jpegFile);
        return NULL;
    }

    fseek(fp, 0, SEEK_SET);
    if(fread(jpegData, 1, dataLength, fp) != dataLength){
        fclose(fp);
        free(jpegData);
        PyErr_Format(PyExc_IOError, "Read file \"%s\" with error", jpegFile);
        return NULL;
    }

    fclose(fp);

    m_handle->ensureThread(PyThread_get_thread_ident());
    auto img = m_handle->decode((const unsigned char*)jpegData, dataLength);

    free(jpegData);

    unsigned char* data = img->buffer();

    npy_intp dims[3] = {(npy_intp)(img->height), (npy_intp)(img->width), 3};
    PyObject* temp = PyArray_SimpleNewFromData(3, dims, NPY_UINT8, data);

    PyArray_ENABLEFLAGS((PyArrayObject*) temp, NPY_ARRAY_OWNDATA);
    delete(img);
    return temp;
}

static PyObject* NvJpeg_write(NvJpeg* Self, PyObject* Argvs, PyObject* Kwds)
{
    unsigned char* jpegFile;
    PyArrayObject *vecin;
    unsigned int quality = 70;
    const char* input = "bgr";
    static char *kwlist[] = {(char*)"path", (char*)"image", (char*)"quality", (char*)"input", NULL};
    if (!PyArg_ParseTupleAndKeywords(Argvs, Kwds, "sO!|Is", kwlist, &jpegFile, &PyArray_Type, &vecin, &quality, &input)){
        PyErr_SetString(PyExc_ValueError, "Parse the argument FAILED! You should pass BGR image numpy array!");
        return NULL;
    }

    #ifdef _MSC_VER
    FILE* fp;
    fopen_s(&fp, (const char*)jpegFile, "wb");
    #else
    FILE* fp = fopen((const char*)jpegFile, "wb");
    #endif
    
    if(fp == NULL){
        PyErr_Format(PyExc_IOError, "Cannot open file \"%s\"", jpegFile);
        return NULL;
    }
    
    PyObject* encodeResponse = NvJpeg_encode_impl(Self, vecin, quality, input);
    if(encodeResponse == NULL){
        fclose(fp);
        return NULL;
    }

    char* jpegData;
    Py_ssize_t jpegDataSize;
    if (PyBytes_AsStringAndSize(encodeResponse, &jpegData, &jpegDataSize) < 0) {
        Py_DECREF(encodeResponse);
        fclose(fp);
        return NULL;
    }
    size_t write_size = fwrite(jpegData, 1, (size_t)jpegDataSize, fp);
    if(write_size != (size_t)jpegDataSize){
        PyErr_Format(PyExc_IOError, "Write file \"%s\" with error", jpegFile);
    }
    Py_DECREF(encodeResponse);
    fclose(fp);
    return Py_BuildValue("l", (long)jpegDataSize);
}


static PyMethodDef NvJpeg_MethodMembers[] =
{
        {"encode",  (PyCFunction)NvJpeg_encode,  METH_VARARGS | METH_KEYWORDS,  "encode jpeg"},
        {"decode", (PyCFunction)NvJpeg_decode, METH_VARARGS,  "decode jpeg"},
        {"read", (PyCFunction)NvJpeg_read, METH_VARARGS,  "read jpeg file and decode"},
        {"write", (PyCFunction)NvJpeg_write, METH_VARARGS | METH_KEYWORDS,  "encode and write jpeg file"},
        {NULL, NULL, 0, NULL}
};


static PyTypeObject NvJpeg_ClassInfo =
{
        PyVarObject_HEAD_INIT(NULL, 0)
        "nvjpeg.NvJpeg",
        sizeof(NvJpeg),
        0
};


void NvJpeg_module_destroy(void *_){
    JpegCoder::cleanUpEnv();
}

static PyModuleDef ModuleInfo =
{
        PyModuleDef_HEAD_INIT,
        "NvJpeg Module",
        "NvJpeg by Nvjpeg",
        -1,
        NULL, NULL, NULL, NULL,
        NvJpeg_module_destroy
};

PyMODINIT_FUNC
PyInit__nvjpeg(void) {
    PyObject * pReturn = NULL;

    NvJpeg_ClassInfo.tp_dealloc   = NvJpeg_Destruct;
    NvJpeg_ClassInfo.tp_repr      = NvJpeg_Repr;
    NvJpeg_ClassInfo.tp_str       = NvJpeg_Str;
    NvJpeg_ClassInfo.tp_flags     = Py_TPFLAGS_DEFAULT;
    NvJpeg_ClassInfo.tp_doc       = "NvJpeg Python Objects---Extensioned by nvjpeg";
    NvJpeg_ClassInfo.tp_weaklistoffset = 0;
    NvJpeg_ClassInfo.tp_methods   = NvJpeg_MethodMembers;
    NvJpeg_ClassInfo.tp_members   = NvJpeg_DataMembers;
    NvJpeg_ClassInfo.tp_dictoffset = 0;
    NvJpeg_ClassInfo.tp_init      = NvJpeg_init;
    NvJpeg_ClassInfo.tp_new = PyType_GenericNew;

    if(PyType_Ready(&NvJpeg_ClassInfo) < 0) 
        return NULL;

    pReturn = PyModule_Create(&ModuleInfo);
    if(pReturn == NULL)
        return NULL;

    Py_INCREF(&ModuleInfo);

    Py_INCREF(&NvJpeg_ClassInfo);
    if (PyModule_AddObject(pReturn, "NvJpeg", (PyObject*)&NvJpeg_ClassInfo) < 0) {
        Py_DECREF(&NvJpeg_ClassInfo);
        Py_DECREF(pReturn);
        return NULL;
    }

    import_array();
    return pReturn;
}
