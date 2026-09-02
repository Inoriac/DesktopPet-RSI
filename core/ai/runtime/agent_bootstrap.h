#ifndef DESKTOP_PET_AGENT_BOOTSTRAP_H
#define DESKTOP_PET_AGENT_BOOTSTRAP_H

#include "ai/domain/domain_result.h"
#include "ai/supervisor/supervisor_protocol_types.h"
#include "runtime_types.h"

class AgentRuntimeServices;

class AgentBootstrap {
public:
    static Result<RuntimeStartReport, DomainError> start(
        AgentRuntimeServices& services,
        const RuntimeStartRequest& request);
    static Result<RuntimeStartReport, DomainError> start(
        AgentRuntimeServices& services,
        const RuntimeStartRequest& request,
        const RuntimeCompositionPolicy& composition,
        const RuntimeBootstrap& bootstrap = {});
};

#endif // DESKTOP_PET_AGENT_BOOTSTRAP_H
