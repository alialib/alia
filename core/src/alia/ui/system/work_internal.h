#pragma once

#include <alia/abi/kernel/events.h>
#include <alia/abi/ui/system/work.h>
#include <alia/ui/system/object.h>

namespace alia {

bool
evaluate_refresh_hook_policy(ui_system& ui, alia_ui_refresh_hook_policy mode);

void
apply_refresh_hook_policy(ui_system& ui, alia_ui_refresh_hook_policy mode);

void
run_layout_resolve(ui_system& ui);

void
update_hot_from_pointer(ui_system& ui);

void
deliver_queued_event(ui_system& ui, alia_event& event);

void
drain_event_queue(ui_system& ui);

} // namespace alia
