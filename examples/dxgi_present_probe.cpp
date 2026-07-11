// DXGI waitable-swapchain present probe.
//
// Native Win32 + D3D11 host with a client-area child HWND as the swapchain
// target and DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT for pacing.
// Esc quits; F11 toggles borderless fullscreen.

#include <alia/platforms/win32/host.h>

#include <cstdio>

int
main()
{
    alia_win32_host* host = alia_win32_host_create();
    if (!host)
    {
        std::fprintf(stderr, "failed to allocate win32 host\n");
        return 1;
    }

    alia_win32_host_open_config const open = {
        .title = "Alia DXGI Present Probe",
        .window_state = alia_window_state_make(1280, 720),
        .resizable = true,
        .vsync = true,
    };
    if (!alia_win32_host_open(host, &open))
    {
        std::fprintf(stderr, "alia_win32_host_open failed\n");
        alia_win32_host_destroy(host);
        return 1;
    }

    alia_win32_host_run_config const run = {
        .ui = nullptr,
        .frame = {},
        .continuous = true,
        .probe_clear = true,
    };
    alia_win32_host_run(host, &run);
    alia_win32_host_destroy(host);
    return 0;
}
