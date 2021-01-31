#pragma once 

/////////////////////////////
///         General Application Class
/////////////////////////////

#include <raylib.h>
#include <iostream>
#include <string>
#include <memory>
#include <vector>





namespace vis
{

    class StateManager;


    class Application
    {
        public:
            Application(int w, int h) ;
            ~Application();
            //On App Startup
            void Start() noexcept;
            //App Update function       
            void Update();      
            //Handle user events
            void Events();
            //Handle Rendering       
            void Render(); 
        public:
            /////// state Handling
            void ChangeState(StateManager* state);
            void PushState(StateManager* state);
            void PopState();
            void CleanStack();
            Vector2 GetScreenCords() noexcept;

            int ScreenWidth,ScreenHeight; 


        private:
            inline void HandleScreen() noexcept; 
            std::vector<StateManager*> states;




    };
} // namespace vis

