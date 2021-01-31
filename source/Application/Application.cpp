#include "Application.hpp"
#include "AppStateManager.hpp"
#include "MenuState.hpp"
#include "OptionsState.hpp"


///////////////////////////////////////
//  TODO: Organize as such that i never ever have to touch that .cpp again
///////////////////////////////////////

vis::Application::Application(int w, int h) : ScreenWidth(w) , ScreenHeight(h){
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(ScreenWidth,ScreenHeight,"Audio Vis");
    SetTargetFPS(75);
}

vis::Application::~Application(){
    this->CleanStack();
    printf("Stack Cleaned\n");
    CloseWindow();
}

//////////////////////////////////////
//  TODO:  Maybe constructor with already initialized instance instead?
/////////////////////////////////////


void vis::Application::Start() noexcept{
    PushState(vis::MenuState::Instance());  
    while(!WindowShouldClose())
    {
        HandleScreen();
        this->Events();
        this->Update();
        this->Render();
    }
}

////////////////////////////////
//  TODO: All of that garbage shouldn't rely on being singleton at all
///////////////////////////////

void vis::Application::HandleScreen() noexcept{
    if (IsWindowResized()){
        ScreenHeight = GetScreenHeight();
        ScreenWidth  = GetScreenWidth ();
    }
}

Vector2 vis::Application::GetScreenCords() noexcept{
    return Vector2{
        static_cast<float>(GetScreenWidth() ),
        static_cast<float>(GetScreenHeight())};
}

void vis::Application::Events() {
    states.back()->Events(this);
}


void vis::Application::Update() {
    states.back()->Update(this);
}

void vis::Application::Render() {
    states.back()->Render(this);
}

void vis::Application::ChangeState(StateManager* state){
    if (!states.empty()){
        states.back()->Cleanup();
        states.pop_back();
    }
    states.emplace_back(state);
    states.back()->Init();
}

void vis::Application::PushState(StateManager* state){
    if(!states.empty()){
        //consider pausing
    }
    states.emplace_back(state);
    states.back()->Init();
}

void vis::Application::PopState(){
    if (!states.empty()){
        states.back()->Cleanup();
        states.pop_back();
    }
    if (!states.empty()){
        //consider resuming
    }
}

inline void vis::Application::CleanStack(){
    while(!states.empty()){
        states.back()->Cleanup();
        states.pop_back();
    }
}