#ifndef CASRAWSYSTEM_H
#define CASRAWSYSTEM_H

#include <map>
#include <memory>
#include <string>

#include "CASSystemImpl.h"

namespace anycas {

class CASSystemFactory{
public:
	CASSystemFactory();
	virtual ~CASSystemFactory();

	static std::shared_ptr<CASSystemImpl> getSystem(const std::string& ocdmId);


private:
	static std::map<std::string, std::weak_ptr<CASSystemImpl::UnderlyingSystem> > systems_;
};

}

#endif /* CASRAWSYSTEM_H */

