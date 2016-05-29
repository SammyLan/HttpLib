#pragma once
#include <functional>

#ifdef WY_CHECKABLE
template <class C>
struct Trackable
{
	Trackable() { ++count_; }
	~Trackable() { --count_; }
	static volatile size_t count_;
};
template <class C>
volatile size_t Trackable<C>::count_ = 0;
#else
template <class C>
struct Trackable
{
};
#endif // WY_CHECKABLE



struct CTask:Trackable<CTask>
{
	virtual void Run() = 0;
	virtual ~CTask(){}
};

template <class TFunc>
class TaskImpl:public CTask
{
public:
	TaskImpl(TFunc const &func):func_(func) {}
	virtual void Run() { func_(); }
private:
	TFunc func_;
};


template<class TFunc>
CTask * NewTaskImpl(TFunc const & func)
{
	return new TaskImpl<TFunc>(func);
}

#define NewTask(FUNC,...) NewTaskImpl(std::bind(FUNC, __VA_ARGS__))
