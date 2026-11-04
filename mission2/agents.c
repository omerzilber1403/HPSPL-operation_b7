#include "agents.h"

Agent *find_agent(Agent *agents, size_t count, int id)
{
    if (agents == NULL || count == 0)
        return NULL;

    Agent *end = agents + count;
    for (Agent *p = agents; p < end; ++p) {
        if (p->id == id)
            return p; /* pointer to the original array element */
    }
    return NULL;
}

int copy_agents(Agent *dest, size_t capacity, const Agent *source, size_t count)
{
    if (count == 0)
        return 1;
    if (dest == NULL || source == NULL || capacity < count)
        return 0;

    for (size_t i = 0; i < count; ++i) {
        const Agent *from = &source[i];
        Agent *to = &dest[i];
        *to = *from; /* copy the whole struct by value */
    }
    return 1;
}

int update_agent(Agent *agent, AgentPatch patch)
{
    if (agent == NULL)
        return 0;

    if (patch.agent_id != agent->id)
        return 0;

    if (patch.new_clearance < 0 || patch.new_clearance > 5)
        return 0;

    if (patch.new_active != 0 && patch.new_active != 1)
        return 0;

    Agent copy = *agent;
    copy.clearance = patch.new_clearance;
    copy.active = patch.new_active;
    *agent = copy;
    return 1;
}

int apply_patch(Agent *agents, size_t count, const AgentPatch *patch)
{
    if (agents == NULL || patch == NULL || count == 0)
        return 0;
    if (patch->new_clearance < 0 || patch->new_clearance > 5 ||
        (patch->new_active != 0 && patch->new_active != 1))
        return 0;

    Agent *target = find_agent(agents, count, patch->agent_id);
    if (target == NULL)
        return 0;

    return update_agent(target, *patch);
}

int classify_access(const Agent *agents, size_t count, const AccessEvent *event)
{
    if (event == NULL)
        return ACCESS_INVALID;

    if (agents == NULL && count != 0)
        return ACCESS_INVALID;

    if (event->agent_id <= 0)
        return ACCESS_INVALID;

    if (event->resource_id != BGU_CAMERAS &&
        event->resource_id != CONTROL_SYSTEM &&
        event->resource_id != AIR_SUPPORT &&
        event->resource_id != EVIL_MORTY_PRIVATE_COMMS &&
        event->resource_id != SERVER_BACKUP &&
        event->resource_id != PORT_8080)
        return ACCESS_INVALID;

    if (event->required_clearance < 0 || event->required_clearance > 5)
        return ACCESS_INVALID;

    if (event->granted != 0 && event->granted != 1)
        return ACCESS_INVALID;

    if (event->sequence < -1 || event->sequence > 7)
        return ACCESS_INVALID;

    if (event->encoded_symbol < -1 || event->encoded_symbol > 25)
        return ACCESS_INVALID;

    /* look for the agent (own loop, because agents is const) */
    int authorized = 0;
    for (size_t i = 0; i < count; ++i) {
        const Agent *p = &agents[i];
        if (p->id == event->agent_id) {
            if (p->active == 1 && p->clearance >= event->required_clearance)
                authorized = 1;
            break;
        }
    }

    if (authorized && event->granted)
        return ACCESS_OK;
    if (!authorized && !event->granted)
        return ACCESS_DENIED;
    if (authorized && !event->granted)
        return ACCESS_UNEXPECTED_DENIAL;
    return ACCESS_UNAUTHORIZED_GRANTED;
}

int recover_code(const Agent *agents, size_t agent_count, const AccessEvent *events, size_t event_count, char *code, size_t capacity)
{
    /* without an output buffer there is nothing to write */
    if (code == NULL)
        return 0;

    /* on any failure the output is an empty string */
    if (capacity > 0)
        code[0] = '\0';

    if (capacity < 9)
        return 0;

    if (agents == NULL && agent_count != 0)
        return 0;

    if (events == NULL && event_count != 0)
        return 0;

    char letters[8];
    int found[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    for (size_t i = 0; i < event_count; ++i) {
        const AccessEvent *event = &events[i];

        /* a damaged or unrelated event is not a code record: skip it */
        if (event->agent_id != 67)
            continue;

        if (classify_access(agents, agent_count, event) != ACCESS_UNAUTHORIZED_GRANTED)
            continue;

        if (event->sequence < 0 || event->sequence > 7)
            continue;

        if (event->encoded_symbol < 0 || event->encoded_symbol > 25)
            continue;

        int position = event->sequence;

        /* the same position twice among valid records: fail */
        if (found[position] == 1)
            return 0;

        found[position] = 1;
        letters[position] = (char)('A' + ((event->encoded_symbol - position + 26) % 26));
    }

    /* every position 0-7 must appear exactly once */
    for (int position = 0; position < 8; ++position) {
        if (found[position] == 0)
            return 0;
    }

    for (int position = 0; position < 8; ++position)
        code[position] = letters[position];
    code[8] = '\0';
    return 1;
}
