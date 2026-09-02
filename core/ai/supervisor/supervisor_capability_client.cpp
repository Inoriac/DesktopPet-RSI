#include "supervisor_capability_client.h"

#include "supervisor_runtime_adapter.h"

SupervisorCapabilityClient::SupervisorCapabilityClient(
    SupervisorRuntimeAdapter* adapter)
    : m_adapter(adapter) {}

Result<CapabilityResult, DomainError> SupervisorCapabilityClient::invoke(
    const CapabilityInvocation& invocation) const {
    if (!m_adapter) {
        return Result<CapabilityResult, DomainError>::failure(
            domainError(QStringLiteral("SUPERVISOR_NOT_READY"),
                        QStringLiteral("supervisor capability client is unavailable")));
    }
    return m_adapter->invokeCapability(invocation);
}
