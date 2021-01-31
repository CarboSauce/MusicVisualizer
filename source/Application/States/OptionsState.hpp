#pragma once
#include "AppStateManager.hpp"
#include <string>

namespace vis{
    class OptionsState : public StateManager {
      public:
            
            void Init();
            void Cleanup();
            
            ///////////////////////////
            
            void Events(Application* app);
            void Update(Application* app);
            void Render(Application* app);
            static StateManager* Instance(){
                return &instance;
            };
        protected:
            OptionsState(){};
        private:
            std::string optionText{"Options"};
            static OptionsState instance;
    };

    
}