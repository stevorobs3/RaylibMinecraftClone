#include "doctest.h"


#include "vector.h"
#include "raylib.h"

TEST_CASE("Vector3") {

	Vector3Int a{1,2,3};
	Vector3 b{3.9,4.9,5.9};
  Vector3Int c{4,6,8};
  // Vector3 x,y,z all get floored
  CHECK(a + b == c);

  CHECK(a + c == Vector3Int(5,8,11));

  CHECK(a.to_string() == "1,2,3");
  CHECK(c.to_string() == "4,6,8");
}
