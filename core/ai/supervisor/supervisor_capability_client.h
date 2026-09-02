#ifndef DESKTOP_PET_SUPERVISOR_CAPABILITY_CLIENT_H
#define DESKTOP_PET_SUPERVISOR_CAPABILITY_CLIENT_H

#include "supervisor_protocol_types.h"

class SupervisorRuntimeAdapter;

class SupervisorCapabilityClient {
public:
    explicit SupervisorCapabilityClient(SupervisorRuntimeAdapter* adapter);

    Result<CapabilityResult, DomainError> invoke(
        const CapabilityInvocation& invocation) const;

private:
    SupervisorRuntimeAdapter* m_adapter = nullptr;
};

#endif // DESKTOP_PET_SUPERVISOR_CAPABILITY_CLIENT_H
