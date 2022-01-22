#include <iostream>
#include <string>
#include <vector>

template <typename... Mixins>
class Point : public Mixins... {
 public:
  double x, y;
  Point() : Mixins()..., x(0.0), y(0.0) {}
  Point(double x, double y) : Mixins()..., x(x), y(y) {}
  void Display() const { std::cout << "(" << x << ", " << y << ")"; }
};

class Label {
 public:
  std::string label;
  Label() : label("Y") {}
};

class Color {
 public:
  unsigned char red = 0, green = 0, blue = 0;
};
using MyPoint = Point<Label, Color>;

template <typename... Mixins>
class Polygon {
 private:
  std::vector<Point<Mixins...>> points_;

 public:
  // public operations
  void ForEach() const {
    for (const auto& p : points_) {
      p.Display();
      std::cout << static_cast<const Label*>(&p)->label << "\n";
    }
  }
  void PushBack(Point<Mixins...> point) { points_.push_back(point); }
};

int main() {
  Polygon<Label, Color> poly;
  MyPoint p1(1.0, 2.0);
  poly.PushBack(p1);
  poly.ForEach();
  return 0;
}