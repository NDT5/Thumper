/*
Author: Nathan Dunn
Module: COR

See associated header file for more information
*/

#include <algorithm>
#include "COR_Event.h"

/*
The default values in this constructor are intended to be kept like this.
*/
COR::Event::Event():
	tickToCall{ std::numeric_limits<decltype(tickToCall)>::max() },
	//referenceCount{ 0 },
	pending{ true },
	markedForDeletion{ false },
	IDForPublicAccess{ std::numeric_limits<decltype(IDForPublicAccess)>::max() }
{
}

/*
The constructor starts the internal thread.
*/
COR::EventEngine::EventEngine():
	eventPointers{},
	events{},
	tickPeriod_ms{ 25 }, // 40 Hz
	safeExit{ false },
	nextIDForPublicAccess{ 0 },
	needToAddEvents{ false }
{
	events.reserve(500); // default preallocation for events
	eventThread = std::thread(&COR::EventEngine::ThreadCallback, this, 0);

	mutex_events.lock();
	eventsSize.store(events.size());
	mutex_events.unlock();

}

/*
Thread safe tick get
*/
unsigned long long COR::EventEngine::GetTicksNow()
{
	return tickNow.load(std::memory_order_consume);
}

/*
Modifying eventPointers from outside the event engine's thread is forbidden, so this sets a flag for that thread to do it. A side effect is that the events don't get removed from events until the time they were scheduled to run. It is paying a memory price to avoid another search operation.
*/
void COR::EventEngine::DeleteEventFromActive(int id)
{
	mutex_events.lock();
	for (size_t i = 0; i < events.size(); i++)
	{
		if (events[i]->IDForPublicAccess == id)
		{
			events[i]->markedForDeletion = true;
			break;
		}
	}
	mutex_events.unlock();
}

/*
Callback function for the EventEngine's internal thread
*/
void COR::EventEngine::ThreadCallback(unsigned long long startTickAt)
{
	tickNow = startTickAt;

	unsigned long long timeNow;
	unsigned long long timeToIncrementTick = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() + tickPeriod_ms;

	while (!safeExit.load(std::memory_order_relaxed))
	{
		/* 
		Before processing events, we need to safely populate the eventPointers with pending events from the events vector. This is done to keep all size changing operations on eventPointers in this thread to avoid having to lock it while processing events. Realistically, this is a very lightweight task. Events must still be locked, though.
		*/
		mutex_events.lock();

		if (events.size() == 0)
		{
			mutex_events.unlock();
			continue;
		}

		if (events.size() > events.capacity() * 0.9)
		{
			DefragmentAndResize();
		}

		if (needToAddEvents)
		{
			
			for (size_t i = 0; i < events.size(); i++)
			{
				if (events[i]->pending)
				{
					eventPointers.push_back(events[i].get());
					events[i]->pending = false;
					//events[i]->referenceCount++;
				}
			}
			
			needToAddEvents = false;
		}

		mutex_events.unlock();

		timeNow = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		if (timeNow >= timeToIncrementTick)
		{
			tickNow.store(timeNow + 1, std::memory_order_release);
			timeToIncrementTick += tickPeriod_ms.load(std::memory_order_relaxed);

			// do per tick stuff
			
			std::sort(
				eventPointers.begin(), 
				eventPointers.end(), 
				[](const COR::Event* a, const COR::Event* b) {return a->tickToCall < b->tickToCall; }
			);
			eventPointers.sort([](const COR::Event* a, const COR::Event* b)
				{
					return a->tickToCall < b->tickToCall;
				}
			);

			size_t itemsToClear = 0;
			for (auto iter = eventPointers.begin(); iter != eventPointers.end(); iter++)
			{
				if ((*iter)->tickToCall <= tickNow)
				{
					if (!(*iter)->markedForDeletion)
					{
						(*iter)->callBack();
						itemsToClear++;
					}
					else
					{
						itemsToClear++;
					}
				}
				else
				{
					break; // everything is sorted by tickToCall, so once the events to be processed on this tick are done, break
				}
			}

			// which events have been called is unordered, which is why we store back references!
			auto iter = eventPointers.begin();
			//for (size_t i = 0; i < itemsToClear && item != eventPointers.end(); i++) // implementation note: .end() doesn't contain anything, it's just an iterator abstraction
			for (size_t i = 0; i < itemsToClear; i++)
			{
				//(*item)->referenceCount--;
				(*iter)->pending = false; // for preventing readding to eventPointers
				(*iter)->markedForDeletion = true; // for defragmenting
				iter = eventPointers.erase(iter);
			}
			
		}
	}
}

/*
This function is meant to be called only when the events vector is nearly full. It does two things:
1) it defragments the events vector and deletes every event that is markedForDeletion
2) it reserves more memory if defragmenting doesn't free up at least 20% of the total capacity
Note: this does NOT lock the mutex_events! That is the responsibility of the caller!
*/
void COR::EventEngine::DefragmentAndResize()
{
	
	if (events.empty())
	{
		// no need to update eventsSize because the defragmenting algorithm didn't run
		return;
	}

	size_t front = 0;
	size_t back = events.size() - 1;
	while (front < back)
	{
		if (!events[front]->markedForDeletion)
		{
			front++;
			break;
		}
		while (front < back && events[back]->markedForDeletion)
		{
			--back;
		}
		if (front == back)
		{
			break;
		}
		std::swap(events[front], events[back]);
		++front;
		--back;
	}

	if (events[front]->markedForDeletion)
	{
		events.resize(front);
	}
	else
	{
		events.resize(front + 1);
	}

	if (events.size() > static_cast<size_t>(events.capacity() * 0.8))
	{
		events.reserve(events.capacity() * 2);
	}
	eventsSize.store(events.size());
	
}