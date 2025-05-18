#Requires -Version 4.0
function Get-MrDscLog {

<#
.SYNOPSIS
    Retrieves information from the DSC operational event log on the specified host.
 
.DESCRIPTION
    The Get-MrDscLogs function retrieves information from the DSC operational event log on
    the specified host(s). PowerShell remoting must be enabled on the specified hosts.
 
.PARAMETER ComputerName
    One or more computer names to retrieve the DSC operational event logs from.

.PARAMETER MaxEvents
    Specifies the maximum number of events to return.

.EXAMPLE
     Get-MrDscLogs -ComputerName Server01 -MaxEvents 12
 
.INPUTS
    None
 
.OUTPUTS
    PSCustomObject
 
.NOTES
    Author:  Mike F Robbins
    Website: http://mikefrobbins.com
    Twitter: @mikefrobbins
#>
    
    [CmdletBinding()]
    param (
        [string[]]$ComputerName,
        [int]$MaxEvents
    )

    Invoke-Command -ComputerName $ComputerName {
        Get-WinEvent –LogName 'Microsoft-Windows-Dsc/Operational' -MaxEvents $Using:MaxEvents |
        Select-Object -Property TimeCreated, Id, LevelDisplayName, Message
    } | Select-Object -Property TimeCreated, Id, LevelDisplayName, Message
}