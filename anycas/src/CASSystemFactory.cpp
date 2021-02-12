
#include "CASSystemFactory.h"
#include "include/CASSystemFactory.h"
#include "rdk_debug.h"

namespace anycas {
	

CASSystemFactory::CASSystemFactory() {
}

CASSystemFactory::~CASSystemFactory() {
	
}

std::map<std::string, std::weak_ptr<CASSystemImpl::UnderlyingSystem> > CASSystemFactory::systems_;

std::shared_ptr<CASSystemImpl> CASSystemFactory::getSystem(const std::string& ocdmId) {

	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Request for %s system, %d stored \n", __FUNCTION__, __LINE__, ocdmId.c_str(), systems_.size());
	auto where = systems_.find(ocdmId);
	
	bool exists = where != systems_.end();
	bool mustMake = !exists;
	if (!mustMake) {
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Found candidate, checking\n", __FUNCTION__, __LINE__);
		mustMake = where->second.expired();
	}

	std::shared_ptr<CASSystemImpl::UnderlyingSystem> underlying;

	if (mustMake) {
		struct OpenCDMSystem*  ocdm_system = opencdm_create_system(ocdmId.c_str());
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Got OpenCDMSystem at %p -- ", __FUNCTION__, __LINE__, ocdm_system);
		underlying = std::make_shared<CASSystemImpl::UnderlyingSystem>(ocdm_system);
		if (exists) {
			where->second = std::weak_ptr<CASSystemImpl::UnderlyingSystem>(underlying);
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Updated\n", __FUNCTION__, __LINE__);
		} 
		else {
			systems_.insert(std::make_pair(ocdmId, std::weak_ptr<CASSystemImpl::UnderlyingSystem>(underlying)) );
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Created\n", __FUNCTION__, __LINE__);
		}
	}
	else {
		underlying = where->second.lock();
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Found OpenCDMSystem at %p [choice of %d]\n", __FUNCTION__, __LINE__, underlying, systems_.size());
	}
	return std::make_shared<CASSystemImpl>(underlying);
}

} // namespace
