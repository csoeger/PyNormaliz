#include <Python.h>
using namespace std;

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include <string>
using std::string;

#include <libnormaliz/cone.h>
#include <libnormaliz/map_operations.h>

using libnormaliz::Cone;
//using libnormaliz::ConeProperty;
using libnormaliz::ConeProperties;
using libnormaliz::Sublattice_Representation;
using libnormaliz::Type::InputType;

#include <vector>
using std::map;
using std::vector;
using std::pair;

// Macros for try catch
#define FUNC_BEGIN try {

#define FUNC_END \
    } catch (libnormaliz::NormalizException& e) { \
        PyErr_SetString( NormalizError, e.what() ); \
        return NULL; \
    } catch( ... ) { \
        PyErr_SetString( PyNormalizError, "unknown exception" ); \
        return NULL; \
    }

static PyObject * NormalizError;
static PyObject * PyNormalizError;
static char* cone_name = "Cone";
static string cone_name_str( cone_name );


typedef int py_size_t;

string PyUnicodeToString( PyObject* in ){
  string out = "";
  int length = PyUnicode_GET_SIZE( in );
  for( int i = 0; i < length; i++ ){
      out += PyUnicode_READ_CHAR( in, i );
  }
  return out;
}

// Converting MPZ's to PyLong and back via strings. Worst possible solution ever.

bool PyLongToNmz( PyObject * in, mpz_class& out ){
  PyObject * in_as_string = PyObject_Str( in );
  const char* in_as_c_string = PyUnicodeToString( in_as_string ).c_str();
  out.set_str( in_as_c_string, 10 );
  return true;
}

PyObject* NmzToPyLong( mpz_class in ){
  string mpz_as_string = in.get_str();
  const char* mpz_as_c_string = mpz_as_string.c_str();
  char * pend;
  PyObject* ret_val = PyLong_FromString( mpz_as_c_string, &pend, 10 );
  return ret_val;
}

PyObject* NmzToPyList( mpq_class in ){
  PyObject* out_list = PyList_New( 2 );
  PyList_SetItem( out_list, 0, NmzToPyLong( in.get_num() ) );
  PyList_SetItem( out_list, 1, NmzToPyLong( in.get_den() ) );
  return out_list;
}

bool PyLongToNmz( PyObject* in, long long & out ){
  
  int overflow;
  out = PyLong_AsLongLongAndOverflow( in, &overflow );
  if( overflow == -1 )
    return false;
  return true;
  
}

PyObject* NmzToPyLong( long long in ){
  
  return PyLong_FromLongLong( in );
  
}

PyObject* NmzToPyLong( libnormaliz::key_t in ){
  
  return PyLong_FromLong( in );
  
}

PyObject* NmzToPyLong( size_t in ){
  
  return PyLong_FromLong( in );
  
}

PyObject* NmzToPyLong( long in ){
  
  return PyLong_FromLong( in );
  
}

template<typename Integer>
bool PyLongToNmz(Integer& x, Integer &out){
  
  return Integer::unimplemented_function;
  
}

template<typename Integer>
PyObject* NmzToPyLong(Integer &in){
  
  return Integer::unimplemented_function;
  
}

template<typename Integer>
static bool PyListToNmz( vector<Integer>& out, PyObject* in ){
  if (!PyList_Check(in))
        return false;
    const int n = PyList_Size(in);
    out.resize(n);
    for (int i = 0; i < n; ++i) {
        PyObject* tmp = PyList_GetItem(in, i);
        if (!PyLongToNmz(tmp, out[i]))
            return false;
    }
    return true;
}

template<typename Integer>
static bool PyIntMatrixToNmz( vector<vector<Integer> >& out, PyObject* in ){
  if (!PyList_Check( in ) )
        return false;
    const int nr = PyList_Size( in );
    out.resize(nr);
    for (int i = 0; i < nr; ++i) {
        bool okay = PyListToNmz(out[i], PyList_GetItem(in, i));
        if (!okay)
            return false;
    }
    return true;
}

template<typename Integer>
PyObject* NmzVectorToPyList(const vector<Integer>& in)
{
    PyObject* vector;
    const size_t n = in.size();
    vector = PyList_New(n);
    for (size_t i = 0; i < n; ++i) {
        PyList_SetItem(vector, i, NmzToPyLong(in[i]));
    }
    return vector;
}

template<typename Integer>
PyObject* NmzMatrixToPyList(const vector< vector<Integer> >& in)
{
    PyObject* matrix;
    const size_t n = in.size();
    matrix = PyList_New( n );
    for (size_t i = 0; i < n; ++i) {
        PyList_SetItem(matrix, i, NmzVectorToPyList(in[i]));
    }
    return matrix;
}

PyObject* NmzHilbertSeriesToPyList(const libnormaliz::HilbertSeries& HS)
{
    PyObject* return_list = PyList_New( 3 );
    PyList_SetItem(return_list, 0, NmzVectorToPyList(HS.getNum()));
    PyList_SetItem(return_list, 1, NmzVectorToPyList(libnormaliz::to_vector(HS.getDenom())));
    PyList_SetItem(return_list, 2, NmzToPyLong(HS.getShift()));
    return return_list;
}

template<typename Integer>
PyObject* NmzHilbertQuasiPolynomialToPyList(const libnormaliz::HilbertSeries& HS)
{
    vector< vector<Integer> > HQ = HS.getHilbertQuasiPolynomial();
    const size_t n = HS.getPeriod();
    PyObject* return_list = PyList_New(n+1);
    for (size_t i = 0; i < n; ++i) {
        PyList_SetItem(return_list, i, NmzVectorToPyList(HQ[i]));
    }
    PyList_SetItem(return_list, n+1, NmzToPyLong(HS.getHilbertQuasiPolynomialDenom()));
    return return_list;
}

template<typename Integer>
PyObject* NmzTriangleListToPyList(const vector< pair<vector<libnormaliz::key_t>, Integer> >& in)
{
    const size_t n = in.size();
    PyObject* M = PyList_New( n );
    for (size_t i = 0; i < n; ++i) {
        // convert the pair
        PyObject* pair = PyList_New(2);
        PyList_SetItem(pair, 0, NmzVectorToPyList<libnormaliz::key_t>(in[i].first));
        PyList_SetItem(pair, 1, NmzToPyLong(in[i].second));
        PyList_SetItem(M, i, pair);
    }
    return M;
}


template<typename Integer>
void delete_cone( PyObject* cone ){
  Cone<Integer> * cone_ptr = reinterpret_cast<Cone<Integer>* >( PyCapsule_GetPointer( cone, cone_name ) );
  delete cone_ptr;
}

template<typename Integer>
Cone<Integer>* get_cone( PyObject* cone ){
  return reinterpret_cast<Cone<Integer>*>( PyCapsule_GetPointer( cone, cone_name ) );
}

template<typename Integer>
PyObject* pack_cone( Cone<Integer>* C ){
  return PyCapsule_New( reinterpret_cast<void*>( C ), cone_name, &delete_cone<Integer> );
}

bool is_cone( PyObject* cone ){
  if( PyCapsule_CheckExact( cone ) ){
    // compare as string
    return cone_name_str == string(PyCapsule_GetName( cone ));
  }
  return false;
}

template<typename Integer>
static PyObject* _NmzConeIntern(PyObject * input_list)
{
    map <InputType, vector< vector<Integer> > > input;
    const int n = PyList_Size(input_list);
    if (n&1) {
        cerr << "Input list must have even number of elements" << endl;
        return Py_False;
    }
    for (int i = 0; i < n; i += 2) {
        PyObject* type = PyList_GetItem(input_list, i);
        if (!PyUnicode_Check(type)) {
            cerr << "Element " << i+1 << " of the input list must be a type string" << endl;
            return Py_False;
        }
        
        string type_str = PyUnicodeToString( type );
        
        PyObject* M = PyList_GetItem(input_list, i+1);
        vector<vector<Integer> > Mat;
        bool okay = PyIntMatrixToNmz(Mat, M);
        if (!okay) {
            cerr << "Element " << i+2 << " of the input list must integer matrix" << endl;
            return Py_False;
        }

        input[libnormaliz::to_type(type_str)] = Mat;
    }

    Cone<Integer>* C = new Cone<Integer>(input);
    
    PyObject* return_container = pack_cone( C );
    
    return return_container;
}

PyObject* _NmzCone(PyObject* self, PyObject* args)
{
    FUNC_BEGIN
    
    PyObject* input_list = PyTuple_GetItem( args, 0 );
    
    if (!PyList_Check( input_list ) )
        return Py_False;

    return _NmzConeIntern<mpz_class>(input_list);

    FUNC_END
}

PyObject* _NmzCompute(PyObject* self, PyObject* args)
{
    FUNC_BEGIN
    
    PyObject* cone = PyTuple_GetItem( args, 0 );
    PyObject* to_compute = PyTuple_GetItem( args, 1 );
    
    if (!PyList_Check( to_compute ) )
        PyErr_SetString( PyNormalizError, "wrong input type" );

    ConeProperties propsToCompute;
    // we have a list
    const int n = PyList_Size(to_compute);

    for (int i = 0; i < n; ++i) {
        PyObject* prop = PyList_GetItem(to_compute, i);
        if (!PyUnicode_Check(prop)) {
            cerr << "Element " << i+1 << " of the input list must be a type string";
            return Py_False;
        }
        string prop_str(PyUnicodeToString(prop));
        propsToCompute.set( libnormaliz::toConeProperty(prop_str) );
    }

    Cone<mpz_class>* C = get_cone<mpz_class>( cone );
    ConeProperties notComputed = C->compute(propsToCompute);

    // Cone.compute returns the not computed properties
    // we return a bool, true when everything requested was computed
    return notComputed.none() ? Py_True : Py_False;
    FUNC_END
}

PyObject* NmzHasConeProperty(PyObject* self, PyObject* args)
{
    FUNC_BEGIN
    
    PyObject* cone = PyTuple_GetItem( args, 0 );
    PyObject* prop = PyTuple_GetItem( args, 1 );
    
    libnormaliz::ConeProperty::Enum p = libnormaliz::toConeProperty(PyUnicodeToString( prop ) );

    Cone<mpz_class>* C = get_cone<mpz_class>( cone );
    return C->isComputed(p) ? Py_True : Py_False;

    FUNC_END
}

template<typename Integer>
PyObject* _NmzConePropertyImpl(Cone<Integer>* C, PyObject* prop_obj)
{
    
    string prop = PyUnicodeToString( prop_obj );
    
    // there is no ConeProperty HilbertQuasiPolynomial, it is part of the HilbertSeries
    // FIXME better way?
//     if(prop == string("HilbertQuasiPolynomial")) {
//         C->compute(ConeProperties(libnormaliz::ConeProperty::HilbertSeries));
//         return NmzHilbertQuasiPolynomialToPyList(C->getHilbertSeries());
//     }

    libnormaliz::ConeProperty::Enum p = libnormaliz::toConeProperty(prop);

    ConeProperties notComputed = C->compute(ConeProperties(p));
    if (notComputed.any()) {
        return Py_None;
    }

    switch (p) {
    case libnormaliz::ConeProperty::Generators:
        return NmzMatrixToPyList(C->getGenerators());

    case libnormaliz::ConeProperty::ExtremeRays:
        return NmzMatrixToPyList(C->getExtremeRays());

    case libnormaliz::ConeProperty::VerticesOfPolyhedron:
        return NmzMatrixToPyList(C->getVerticesOfPolyhedron());

    case libnormaliz::ConeProperty::SupportHyperplanes:
        return NmzMatrixToPyList(C->getSupportHyperplanes());

    case libnormaliz::ConeProperty::TriangulationSize:
        return NmzToPyLong(C->getTriangulationSize());

    case libnormaliz::ConeProperty::TriangulationDetSum:
        return NmzToPyLong(C->getTriangulationDetSum());

    case libnormaliz::ConeProperty::Triangulation:
        return NmzTriangleListToPyList<Integer>(C->getTriangulation());

    case libnormaliz::ConeProperty::Multiplicity:
        return NmzToPyList(C->getMultiplicity());

    case libnormaliz::ConeProperty::RecessionRank:
        return NmzToPyLong(C->getRecessionRank());

    case libnormaliz::ConeProperty::AffineDim:
        return NmzToPyLong(C->getAffineDim());

    case libnormaliz::ConeProperty::ModuleRank:
        return NmzToPyLong(C->getModuleRank());

    case libnormaliz::ConeProperty::HilbertBasis:
        return NmzMatrixToPyList(C->getHilbertBasis());

    case libnormaliz::ConeProperty::MaximalSubspace:
        return NmzMatrixToPyList(C->getMaximalSubspace());

    case libnormaliz::ConeProperty::ModuleGenerators:
        return NmzMatrixToPyList(C->getModuleGenerators());

    case libnormaliz::ConeProperty::Deg1Elements:
        return NmzMatrixToPyList(C->getDeg1Elements());

    case libnormaliz::ConeProperty::HilbertSeries:
        return NmzHilbertSeriesToPyList(C->getHilbertSeries());

    case libnormaliz::ConeProperty::Grading:
        {
        vector<Integer> grad = C->getGrading();
        grad.push_back(C->getGradingDenom());
        return NmzVectorToPyList(grad);
        }

    case libnormaliz::ConeProperty::IsPointed:
        return C->isPointed() ? Py_True : Py_False;

    case libnormaliz::ConeProperty::IsDeg1ExtremeRays:
        return C->isDeg1ExtremeRays() ? Py_True : Py_False;

    case libnormaliz::ConeProperty::IsDeg1HilbertBasis:
        return C->isDeg1HilbertBasis() ? Py_True : Py_False;

    case libnormaliz::ConeProperty::IsIntegrallyClosed:
        return C->isIntegrallyClosed() ? Py_True : Py_False;

    case libnormaliz::ConeProperty::OriginalMonoidGenerators:
        return NmzMatrixToPyList(C->getOriginalMonoidGenerators());

    case libnormaliz::ConeProperty::IsReesPrimary:
        return C->isReesPrimary() ? Py_True : Py_False;

    case libnormaliz::ConeProperty::ReesPrimaryMultiplicity:
        return NmzToPyLong(C->getReesPrimaryMultiplicity());

    // StanleyDec is special and we do not support the required conversion at
    // this time. If you really need this, contact the developers.
    case libnormaliz::ConeProperty::StanleyDec:
        //C->getStanleyDec();
        break;

    case libnormaliz::ConeProperty::ExcludedFaces:
        return NmzMatrixToPyList(C->getExcludedFaces());

    case libnormaliz::ConeProperty::Dehomogenization:
        return NmzVectorToPyList(C->getDehomogenization());

    case libnormaliz::ConeProperty::InclusionExclusionData:
        return NmzTriangleListToPyList<long>(C->getInclusionExclusionData());

    case libnormaliz::ConeProperty::ClassGroup:
        return NmzVectorToPyList(C->getClassGroup());

    case libnormaliz::ConeProperty::Sublattice:
        C->compute(p);
        return C->isComputed(p) ? Py_True : Py_False;

//  the following properties are compute options and do not return anything
    case libnormaliz::ConeProperty::DualMode:
    case libnormaliz::ConeProperty::DefaultMode:
    case libnormaliz::ConeProperty::Approximate:
    case libnormaliz::ConeProperty::BottomDecomposition:
    case libnormaliz::ConeProperty::KeepOrder:
        return Py_True;    // FIXME: appropriate value?

    default:
        // Case not handled. Should signal an error
        break;
    }

    return Py_None;
}

PyObject* _NmzConeProperty( PyObject* self, PyObject* args ){
  
  FUNC_BEGIN
  PyObject* cone = PyTuple_GetItem( args, 0 );
  PyObject* prop = PyTuple_GetItem( args, 1 );
  
  if( !is_cone( cone ) ){
    PyErr_SetString( PyNormalizError, "First argument must be a cone" );
    return NULL;
  }
  
  if( !PyUnicode_Check( prop ) ){
    PyErr_SetString( PyNormalizError, "Second argument must be a unicode string" );
    return NULL;
  }
  
  Cone<mpz_class>* C = get_cone<mpz_class>( cone );
  
  return _NmzConePropertyImpl( C, prop );
  FUNC_END
}

PyObject* NmzSetVerboseDefault( PyObject* self, PyObject* args)
{
    FUNC_BEGIN
    PyObject * value = PyTuple_GetItem( args, 0 );
    if (value != Py_True && value != Py_False){
        PyErr_SetString( PyNormalizError, "Argument must be true or false" );
        return NULL;
    }
    return libnormaliz::setVerboseDefault(value == Py_True) ? Py_True : Py_False;
    FUNC_END
}

PyObject* NmzSetVerbose(PyObject* self, PyObject* args)
{
    FUNC_BEGIN
    PyObject* cone = PyTuple_GetItem( args, 0 );
    
    if( !is_cone( cone ) ){
        PyErr_SetString( PyNormalizError, "First argument must be a cone" );
        return NULL;
    }
    
    PyObject* value = PyTuple_GetItem( args, 1 );
    if (value != Py_True && value != Py_False){
        PyErr_SetString( PyNormalizError, "Second argument must be true or false" );
        return NULL;
    }
    bool old_value;
    Cone<mpz_class>* C = get_cone<mpz_class>( cone );
    old_value = C->setVerbose(value == Py_True);
    return old_value ? Py_True : Py_False;
    FUNC_END
}

PyObject* NmzEmbeddingDimension(PyObject* self, PyObject* args)
{
    FUNC_BEGIN
    PyObject* cone = PyTuple_GetItem( args, 0 );
    if (!is_cone(cone)){
        PyErr_SetString( PyNormalizError, "First argument must be a cone" );
        return NULL;
    }
    Cone<mpz_class>* C = get_cone<mpz_class>(cone);
    return NmzToPyLong(C->getEmbeddingDim());
    FUNC_END
}

template<typename Integer>
static PyObject* _NmzBasisChangeIntern(PyObject* cone)
{
    Cone<Integer>* C = get_cone<Integer>(cone);
    Sublattice_Representation<Integer> bc = C->getSublattice();

    PyObject* res = PyList_New( 3 );
    PyList_SetItem(res, 0, NmzMatrixToPyList(bc.getEmbedding()));
    PyList_SetItem(res, 1, NmzMatrixToPyList(bc.getProjection()));
    PyList_SetItem(res, 2, NmzToPyLong(bc.getAnnihilator()));
    // Dim, Rank, Equations and Congruences are already covered by special functions
    // The index is not always computed and not so relevant
    return res;
}

PyObject* _NmzBasisChange(PyObject* self, PyObject* args)
{
    FUNC_BEGIN
    PyObject* cone = PyTuple_GetItem( args, 0 );
    if (!is_cone(cone)){
        PyErr_SetString( PyNormalizError, "First argument must be a cone" );
        return NULL;
    }
    return _NmzBasisChangeIntern<mpz_class>(cone);
    FUNC_END
}

PyObject* NmzRank(PyObject* self, PyObject* args)
{
    FUNC_BEGIN
    PyObject* cone = PyTuple_GetItem( args, 0 );
    if (!is_cone(cone)){
        PyErr_SetString( PyNormalizError, "First argument must be a cone" );
        return NULL;
    }
    Cone<mpz_class>* C = get_cone<mpz_class>(cone);
    return NmzToPyLong(C->getSublattice().getRank());
    FUNC_END
}


PyObject* NmzIsInhomogeneous(PyObject* self, PyObject* args)
{
    FUNC_BEGIN
    PyObject* cone = PyTuple_GetItem( args, 0 );
    if (!is_cone(cone)){
        PyErr_SetString( PyNormalizError, "First argument must be a cone" );
        return NULL;
    }
    Cone<mpz_class>* C = get_cone<mpz_class>(cone);
    return C->isInhomogeneous() ? Py_True : Py_False;
    FUNC_END
}

PyObject* NmzEquations(PyObject* self, PyObject* args)
{
    FUNC_BEGIN
    PyObject* cone = PyTuple_GetItem( args, 0 );
    if (!is_cone(cone)){
        PyErr_SetString( PyNormalizError, "First argument must be a cone" );
        return NULL;
    }
    Cone<mpz_class>* C = get_cone<mpz_class>(cone);
    C->compute(ConeProperties(libnormaliz::ConeProperty::SupportHyperplanes));
    return NmzMatrixToPyList(C->getSublattice().getEquations());
    FUNC_END
}

PyObject* NmzCongruences(PyObject* self, PyObject* args)
{
    FUNC_BEGIN
    PyObject* cone = PyTuple_GetItem( args, 0 );
    if (!is_cone(cone)){
        PyErr_SetString( PyNormalizError, "First argument must be a cone" );
        return NULL;
    }
    Cone<mpz_class>* C = get_cone<mpz_class>(cone);
    C->compute(ConeProperties(libnormaliz::ConeProperty::SupportHyperplanes));
    return NmzMatrixToPyList(C->getSublattice().getCongruences());
    FUNC_END
}


/*
 * Python init stuff
 */

static PyMethodDef PyNormalizMethods[] = {
    {"NmzCone",  _NmzCone, METH_VARARGS,
     "Create a cone"},
    {"NmzCompute", _NmzCompute, METH_VARARGS,
     "Compute some stuff"},
    {"NmzHasConeProperty", NmzHasConeProperty, METH_VARARGS,
     "Check if property is computed "},
    {"NmzConeProperty", _NmzConeProperty, METH_VARARGS,
      "Return cone property" },
    { "NmzSetVerboseDefault", NmzSetVerboseDefault, METH_VARARGS,
      "Set verbosity" },
    { "NmzSetVerbose", NmzSetVerbose, METH_VARARGS,
      "Set verbosity of cone" },
    { "NmzBasisChange", _NmzBasisChange, METH_VARARGS,
      "Get information of basis change" },
    { "NmzIsInhomogeneous", NmzIsInhomogeneous, METH_VARARGS,
      "Is inhomogeneous cone" },
    { "NmzEquations", NmzEquations, METH_VARARGS,
      "Equations" },
    { "NmzCongruences", NmzCongruences, METH_VARARGS,
      "Congruences of cone" },
    { "NmzRank", NmzRank, METH_VARARGS,
      "Get rank" },
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef PyNormalizmodule = {
   PyModuleDef_HEAD_INIT,
   "PyNormaliz",   /* name of module */
   NULL, /* module documentation, may be NULL */
   -1,       /* size of per-interpreter state of the module,
                or -1 if the module keeps state in global variables. */
   PyNormalizMethods
};

PyMODINIT_FUNC
PyInit_PyNormaliz(void)
{
    PyObject * module;
    
    module = PyModule_Create(&PyNormalizmodule);
    
    if(module == NULL){
        return NULL;
    }
    
    NormalizError = PyErr_NewException( "Normaliz.error", NULL, NULL );
    Py_INCREF( NormalizError );
    PyNormalizError = PyErr_NewException( "Normaliz.interface_error", NULL, NULL );
    Py_INCREF( PyNormalizError );
    
    PyModule_AddObject( module, "error", NormalizError );
    PyModule_AddObject( module, "error", PyNormalizError );
    
    return module;
}

/*
 * main
 */

wchar_t *GetWC(const char *c)
{
    const size_t cSize = strlen(c)+1;
    wchar_t* wc = new wchar_t[cSize];
    mbstowcs (wc, c, cSize);

    return wc;
}

int main(int argc, char *argv[]){

#ifdef PYTHON_VERSION_OLDER_THREE_FIVE
    const size_t cSize = strlen(argv[0])+1;
    wchar_t* program = new wchar_t[cSize];
    mbstowcs (program, argv[0], cSize);
#else
    wchar_t *program = Py_DecodeLocale(argv[0], NULL);
    if (program == NULL) {
        fprintf(stderr, "Fatal error: cannot decode argv[0]\n");
        exit(1);
    }
#endif

    /* Add a built-in module, before Py_Initialize */

    PyImport_AppendInittab("PyNormaliz", PyInit_PyNormaliz);
    /* Pass argv[0] to the Python interpreter */
    
    
    Py_SetProgramName(program);

    /* Initialize the Python interpreter.  Required. */
    Py_Initialize();

    /* Optionally import the module; alternatively,
       import can be deferred until the embedded script
       imports it. */
    PyImport_ImportModule("PyNormaliz");

    PyMem_RawFree(program);
    return 0;
}
