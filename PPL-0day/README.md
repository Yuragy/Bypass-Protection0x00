Wrote this a while back. Micro$uck never responded to my email, consider this repsonsible.

Can disable any antivirus at boot. Works with any system protected process but is most applicable to AVs.

A race condition allows us to start an anti-malware service and replace it's access token with one corresponding to a supremely deprivileged security context.

Creates WMI filter/consumer to start on boot before `svchost.exe`: https://github.com/pulpocaminante/PPL-0day/blob/main/AntiAV.hpp

This results in our payload being executed as the SYSTEM user, which then has full privileges to modify the security context of a paused protected child process.

PoC for starting the process paused, replacing the token and resuming it: https://github.com/pulpocaminante/PPL-0day/blob/main/PPL_Start.hpp

The rest of the files are just dependencies.

Requires phnt headers: https://github.com/winsiderss/phnt

For more info on PPLs:

[https://learn.microsoft.com/en-us/windows/win32/services/protecting-anti-malware-services-](https://learn.microsoft.com/en-us/windows/win32/services/protecting-anti-malware-services-)
