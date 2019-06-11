#ifndef ISPH_TIMER_H
#define ISPH_TIMER_H

#include <chrono>

namespace isph {

	/*!
	 *	\class	Timer
	 *	\brief	Measure time intervals in milliseconds.
	 */
	class Timer
	{
	public:

		Timer();
		virtual ~Timer();

		/*!
		 *	\brief	Start the timing.
		 */
		void Start();

		/*!
		 *	\brief	Get the time elapsed, in milliseconds.
		 */
		double Time();

	protected:

      std::chrono::time_point<std::chrono::steady_clock> t_init;
	};


} // namespace isph

#endif
