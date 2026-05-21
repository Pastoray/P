fn printi32(i32 x) -> void;

enum ErrorCode
{
  None,       // 0
  NotFound,   // 1
  Forbidden = 403,
  Internal    // 404
};

struct Result
{
  ErrorCode status;
  union _
  {
    i32 value;
    i32 error_meta;
  } data;
};

fn main() -> void
{
  ErrorCode err = ErrorCode.NotFound; 
  
  i32 code = err;
  printi32(code);

  ErrorCode forbidden = ErrorCode.Forbidden;
  ErrorCode internal = ErrorCode.Internal;
  
  printi32(forbidden); // 403
  printi32(internal);  // 404

  Result res;
  res.status = ErrorCode.None;
  res.data.value = 100;

  if (res.status == 0) 
  {
    printi32(res.data.value);
  }

  return 0;
}
