#pragma once

class QApplication;

// The initial workspace uses the bundled light, comfortable token set.
void applyWorkspaceTheme(QApplication& application);
int workspaceMetric(const char* name);
