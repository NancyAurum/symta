// FFI bindings for our test C library.
//
// The library source is at cffilib/src/cffilib.c; built per
// platform into cffilib/lib/cffi.ffi which run.sh copies into
// the test project's ffi/ folder before invoking symta.
//
// Naming convention matches cffilib.c. Each binding lives in this
// module so individual test cases just `use cffi` and call the
// functions.

ffi_begin local cffi
export
  c_id_i32 c_id_u32 c_id_f32 c_id_f64 c_id_ptr c_id_str
  c_const_i32_max c_const_i32_min c_const_i32_neg1
  c_const_u32_max c_const_f32_pi c_const_f64_pi c_const_ptr_null
  c_add_i32_i32 c_add_u32_u32 c_add_f32_f32 c_add_f64_f64
  c_add_i32_f64 c_add_f64_i32
  c_mix4_iffi c_mix4_fiif c_mix4_ffii c_mix4_iiff
  c_mix6_ifiifi c_mix6_fiiffi
  c_sum_i32_4 c_sum_i32_6 c_sum_i32_8 c_sum_i32_12
  c_sum_f64_4 c_sum_f64_6 c_sum_f64_8 c_sum_f64_10
  c_strlen c_strcmp c_strchr_ofs c_str_bytesum
  c_buf_fill c_buf_checksum c_buf_alloc c_buf_free
  c_buf_set_u32 c_buf_get_u32 c_buf_copy
  c_f64_neg c_f32_neg c_f64_seq
  c_ptr_inc c_ptr_diff
  c_hash_i32 cffi_version

// Identity / round-trip.
ffi c_id_i32.int  X.int
ffi c_id_u32.u4   X.u4
ffi c_id_f32.float  X.float
ffi c_id_f64.double  X.double
ffi c_id_ptr.ptr  P.ptr
ffi c_id_str.text S.text

// Constants.
ffi c_const_i32_max.int
ffi c_const_i32_min.int
ffi c_const_i32_neg1.int
ffi c_const_u32_max.u4
ffi c_const_f32_pi.float
ffi c_const_f64_pi.double
ffi c_const_ptr_null.ptr

// Arithmetic.
ffi c_add_i32_i32.int A.int B.int
ffi c_add_u32_u32.u4  A.u4  B.u4
ffi c_add_f32_f32.float A.float B.float
ffi c_add_f64_f64.double A.double B.double
ffi c_add_i32_f64.double A.int B.double
ffi c_add_f64_i32.double A.double B.int

// Mixed interleaving.
ffi c_mix4_iffi.double A.int B.float C.float D.int
ffi c_mix4_fiif.double A.float B.int C.int D.float
ffi c_mix4_ffii.double A.float B.float C.int D.int
ffi c_mix4_iiff.double A.int B.int C.float D.float
ffi c_mix6_ifiifi.double A.int B.float C.int D.int E.float F.int
ffi c_mix6_fiiffi.double A.float B.int C.int D.float E.float F.int

// Arity coverage.
ffi c_sum_i32_4.int  A.int B.int C.int D.int
ffi c_sum_i32_6.int  A.int B.int C.int D.int E.int F.int
ffi c_sum_i32_8.int  A.int B.int C.int D.int E.int F.int G.int H.int
ffi c_sum_i32_12.int A.int B.int C.int D.int E.int F.int G.int H.int
                     I.int J.int K.int L.int

ffi c_sum_f64_4.double  A.double B.double C.double D.double
ffi c_sum_f64_6.double  A.double B.double C.double D.double E.double F.double
ffi c_sum_f64_8.double  A.double B.double C.double D.double E.double F.double G.double H.double
ffi c_sum_f64_10.double A.double B.double C.double D.double E.double F.double G.double H.double
                     I.double J.double

// String / cstring.
ffi c_strlen.int      S.text
ffi c_strcmp.int      A.text B.text
ffi c_strchr_ofs.int  S.text Ch.int
ffi c_str_bytesum.u4  S.text

// Buffers.
ffi c_buf_fill.void     P.ptr N.int V.int
ffi c_buf_checksum.u4   P.ptr N.int
ffi c_buf_alloc.ptr     N.int
ffi c_buf_free.void     P.ptr
ffi c_buf_set_u32.void  P.ptr Ofs.int V.u4
ffi c_buf_get_u32.u4    P.ptr Ofs.int
ffi c_buf_copy.ptr      Dst.ptr Src.ptr N.int

// Float edge.
ffi c_f64_neg.double  X.double
ffi c_f32_neg.float  X.float
ffi c_f64_seq.double  X.double

// Pointers.
ffi c_ptr_inc.ptr   P.ptr
ffi c_ptr_diff.int  A.ptr B.ptr

// Stress.
ffi c_hash_i32.u4   X.int
ffi cffi_version.text
