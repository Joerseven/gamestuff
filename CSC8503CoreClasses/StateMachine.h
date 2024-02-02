#pragma once

namespace NCL {
	namespace CSC8503 {
		class State;
		class StateTransition;

		typedef std::multimap<State*, StateTransition*> TransitionContainer;
		typedef TransitionContainer::iterator TransitionIterator;

		class StateMachine	{
		public:
			StateMachine();
			~StateMachine(); // Nu uh

			void AddState(State* s);
			void AddTransition(StateTransition* t);

			void Update(float dt); // Nu uh

            float elapsed;
            float duration;

		protected:
			State * activeState;


			std::vector<State*> allStates;
			TransitionContainer allTransitions;
		};
	}
}