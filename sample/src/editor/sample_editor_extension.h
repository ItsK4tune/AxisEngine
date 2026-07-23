#pragma once

#ifdef ENABLE_EDITOR

// Registers both demo extensions under one stable owner. Repeated registration is
// harmless; owner-scoped cleanup removes the module and panel together.
bool RegisterSampleEditorExtension();
void UnregisterSampleEditorExtension();

#endif
