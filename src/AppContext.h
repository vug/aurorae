#pragma once

namespace aur {

class Renderer;
class Application;

class AppContext {
public:
  static const Renderer& getRenderer();

private:
  friend class Application;

  // initialize is meant to be called by Application
  static void initialize(Renderer& renderer);

  static Renderer* renderer_;
};

} // namespace aur