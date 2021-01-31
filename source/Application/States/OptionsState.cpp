#include "OptionsState.hpp"
#include "raylib.h"
#include "AppStateManager.hpp"

vis::OptionsState vis::OptionsState::instance;
/////////////////////////////////////
// TODO: Proper option class lol
/////////////////////////////////////

void vis::OptionsState::Events(Application* app){
    if (IsKeyPressed(KEY_Q)) app->PopState();
}

void vis::OptionsState::Update(Application* app){
    (void)app;
}

void vis::OptionsState::Render(Application* app){
    BeginDrawing();
        ClearBackground(BLACK);
        DrawText(optionText.c_str(),300,300,50,WHITE);
    EndDrawing();

    (void)app;
}

void vis::OptionsState::Init(){

}

void vis::OptionsState::Cleanup(){
    
}


 
