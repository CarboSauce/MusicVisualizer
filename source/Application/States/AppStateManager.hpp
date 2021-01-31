#pragma once
#include "Application.hpp"



namespace vis{
    class StateManager{
        public:
            
            virtual void Init() = 0;
            virtual void Cleanup() = 0;
            
            ///////////////////////////
            
            virtual void Events(Application* app)= 0;
            virtual void Update(Application* app) = 0;
            virtual void Render(Application* app) = 0;

            void ChangeState(vis::Application* app,StateManager* state)
            {
                app->ChangeState(state);
            }
        protected: 
            StateManager(){};
            
            
    };
}