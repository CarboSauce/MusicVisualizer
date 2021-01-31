#pragma once
#pragma once
#include "AppStateManager.hpp"
#include <string>

namespace vis{
    class MenuState : public StateManager {
      public:
             
             void Init();
             void Cleanup();
            
            ///////////////////////////
            
            void Events(Application* app);
            void Update(Application* app);
            void Render(Application* app);
            static StateManager* Instance(){
                return &instance;
            }
      protected:
        MenuState(){};
        
      private:
        std::string text{"Menu"};
        static MenuState instance;
    };
    
}