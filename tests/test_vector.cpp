#include "doctest.h"
#include "vector.h"

TEST_CASE("Integer world coordinates add exactly") {
  Vector3Int a{1, 2, 3};
  Vector3Int b{4, 6, 8};
  CHECK(a + b == Vector3Int(5, 8, 11));
  CHECK(a.to_string() == "1,2,3");
  CHECK(b.to_string() == "4,6,8");
}
