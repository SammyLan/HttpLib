#pragma once
#include <cassert>
#include <boost\noncopyable.hpp>
namespace WYUtil
{

	template<class D>
	class Instance:public boost::noncopyable
	{
	public:
		static D *instance(){return instance_;}
	private:
		static D * instance_;
	protected:
		Instance(){ assert(instance_ == NULL);instance_ = (D*) this;}
		~Instance(){ instance_ = (D*) NULL;}
	};

	template<class D>
	D * Instance<D>::instance_ = NULL;

}