#include "MenuState.hpp"
#include <raylib.h>
#include "AppStateManager.hpp"
#include "Application.hpp"
#include "GameState.hpp"

vis::MenuState vis::MenuState::instance;
///////////////////////////////////
// TODO: A lot.
///////////////////////////////////

void vis::MenuState::Events(Application* app){
    if (IsFileDropped()) app->PushState(vis::GameState::Instance());
}

void vis::MenuState::Update(Application* app){
    (void)app;
}

void vis::MenuState::Render(Application* app){
    int measure = MeasureText(text.c_str(),60);
    BeginDrawing();
        ClearBackground(WHITE);
        DrawText(text.c_str(),app->ScreenWidth/2-measure/2,app->ScreenHeight/2-30,60,BLACK);
    EndDrawing();
}

void vis::MenuState::Init(){

}

void vis::MenuState::Cleanup(){
    printf("Menu Cleanup\n");
}
