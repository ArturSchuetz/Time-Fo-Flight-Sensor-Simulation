#include "CoreSystems/BowBasicTimer.h"

#include <iostream>
#include <string>

int main(int /*argc*/, char* /*argv[]*/)
{
	bow::BasicTimer timer;

	std::cout << "I will now count to ten:" << std::endl;

	// wait 10 seconds
	while (timer.GetTotal() < 10)
	{
		// Update timer to get new total time and new delta time
		timer.Update();

		std::cout << std::to_string(timer.GetTotal()) << "\r";
	}

	std::cout << std::endl;
	std::cout << "I am done!" << std::endl;

	return 0;
}
