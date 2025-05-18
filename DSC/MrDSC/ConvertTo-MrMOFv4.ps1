#Requires -Version 4.0
function ConvertTo-MrMOFv4 {

<#
.SYNOPSIS
    Removes items from a MOF configuration file that are incompatible with PowerShell version 4.
 
.DESCRIPTION
    The ConvertTo-MrMOFv4 function removes specific items from a MOF configuration file that was
    created on a machine running PowerShell version 5 to make the MOF file compatible with a
    machine running PowerShell version 4.
 
.PARAMETER Path
    Path to the MOF configuration files to convert.

.PARAMETER $Pattern
    Hidden parameter that can be used to remove different patterns from a MOF configuration file.

.EXAMPLE
     ConvertTo-MrMOFv4 -Path C:\MofFiles\config.mof

.EXAMPLE
     Get-ChildItem -Path C:\MofFiles | ConvertTo-MrMOFv4
 
.INPUTS
    String
 
.OUTPUTS
    None
 
.NOTES
    Author:  Mike F Robbins
    Website: http://mikefrobbins.com
    Twitter: @mikefrobbins
#>

    [CmdletBinding()]
    param (
        [Parameter(Mandatory,
                   ValueFromPipelineByPropertyName)]
        [ValidateScript({Test-Path $_ -PathType Leaf -Include *.mof})]
        [Alias('FullName')]
        [string[]]$Path,

        [Parameter(DontShow)]
        [ValidateNotNullorEmpty()]
        [string]$Pattern = '^\sName=.*;$|^\sConfigurationName\s=.*;$'
    )

    PROCESS {
        foreach ($file in $Path) {
            
            $mof = Get-Content -Path $file
            
            if ($mof -match $Pattern) {
                Write-Verbose -Message "PowerShell v4 compatibility problems were found in file: $file"

                try {
                    $mof -replace $Pattern |
                    Set-Content -Path $file -Force -ErrorAction Stop
                }
                catch {
                    Write-Warning -Message "An error has occurred. Error details: $_.Exception.Message"
                }
                finally {
                    if ((Get-Content -Path $file) -notmatch $Pattern) {
                        Write-Verbose -Message "The file: $file was successfully modified."
                    }
                    else {
                        Write-Verbose -Message "Attempt to modify the file: $file was unsuccessful."
                    }
                }
            }
        }
    }
}