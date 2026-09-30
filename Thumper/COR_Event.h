/*
Author: Nathan Dunn
Module: COR

Known bugs and limitations:
- none at this time
*/
#pragma once
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <atomic>

namespace COR
{
	class Event;
	class Event_LogEventInfo;
	class EventEngine;
}

class COR::Event
{
	// This class is intended to be a base for inheritance.
public:
	// public member functions
	Event();							// derived class must provide it's own constructor
	virtual ~Event() = default;			// derived classes are expected to override the destructor
	virtual void callBack() = 0;		// event callback function

private:
	// private member functions

public:
	// public member variables
	unsigned long long tickToCall;
	bool pending;
	bool markedForDeletion;
	//int referenceCount;
	unsigned long long IDForPublicAccess;

private:
	// private member variables
};

class COR::Event_LogEventInfo :
	public COR::Event
{
public:
	// public member functions
	Event_LogEventInfo();
	~Event_LogEventInfo() override = default;
	void callBack() override; // IMPORTANT: this is NOT supposed to reference any memory meant to outlive the end of the callback function!

private:
	// private member functions

public:
	// public member variables

private:
	// private member variables
};

class COR::EventEngine
{
public:
	// public member types

private:
	// private member types

public:
	// public member functions
	EventEngine();
	unsigned long long GetTicksNow();
	template <typename EventType, typename... Args> unsigned long long AddEventAndGetID(Args&&... args);
	void DeleteEventFromActive(int id);

private:
	// private member functions
	void ThreadCallback(unsigned long long startTickAt);
	void DefragmentAndResize();

public:
	// public member variables

private:
	// private member variables
	std::list<COR::Event*> eventPointers;
	//std::list<size_t> eventsToMoveToEventPointers;
	std::thread eventThread;
	unsigned long long nextIDForPublicAccess;
	bool needToAddEvents;

	// thread safe data
	std::atomic<unsigned long long> tickPeriod_ms; // excessive, but time will be cast to this later, so it makes sense to use this type to avoid a cast
	std::atomic<unsigned long long> tickNow; // read only except for the ThreadCallback
	std::atomic<bool> safeExit;
	std::mutex mutex_events;
	std::vector<std::unique_ptr<COR::Event>> events;
	std::atomic<size_t> eventsSize;
};

template <typename EventType, typename... Args> unsigned long long COR::EventEngine::AddEventAndGetID(Args&&... args)
{
	static_assert(std::is_base_of_v<COR::Event, EventType>, "EventType must inherit from COR::Event");

	needToAddEvents = true;

	unsigned long long id = nextIDForPublicAccess;
	nextIDForPublicAccess++;

	mutex_events.lock();
	events.emplace_back(std::make_unique<EventType>(std::forward<Args>(args)...));
	events.back()->IDForPublicAccess = id;
	eventsSize.store(events.size());
	mutex_events.unlock();

	return id;
}