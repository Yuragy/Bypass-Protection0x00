#Requires -Version 4.0
function Publish-MrMOFToSMB {

<#
.SYNOPSIS
    Publishes a DSC MOF configuration file to the pull server that's configured on a target node(s).
 
.DESCRIPTION
    Publish-MrMOFToSMB is an advanced PowerShell function that publishes one or more MOF configuration files
    to the an SMB DSC server by determining the ConfigurationID (GUID) that's configured on the target node along
    with the UNC path of the SMB pull server and creates the necessary checksum along with copying the MOF and
    checksum to the pull server.
 
.PARAMETER ConfigurationPath
    The folder path on the local computer that contains the mof configuration files.

.PARAMETER ComputerName
    The computer name of the target node that the DSC configuration is created for.
 
.EXAMPLE
     Publish-MrMOFToSMB -ConfigurationPath 'C:\MyMofFiles'

.EXAMPLE
     Publish-MrMOFToSMB -ConfigurationPath 'C:\MyMofFiles' -ComputerName 'Server01', 'Server02'

.EXAMPLE
     'Server01', 'Server02' | Publish-MrMOFToSMB -ConfigurationPath 'C:\MyMofFiles'

.EXAMPLE
     MyDscConfiguration -Param1 Value1 -Parm2 Value2 | Publish-MrMOFToSMB
 
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
        [ValidateScript({Test-Path -Path $_ -PathType Container})]
        [Alias('Directory')]
        [string]$ConfigurationPath,

        [Parameter(ValueFromPipeline,
                   ValueFromPipelineByPropertyName)]
        [Alias('BaseName')]
        [string[]]$ComputerName
    )

    BEGIN {
        if (-not($PSBoundParameters['ComputerName'])) {
            $ComputerName = (Get-ChildItem -Path $ConfigurationPath\*.mof).basename
        }
    }

    PROCESS {
        foreach ($Computer in $ComputerName) {

            try {
                Write-Verbose -Message "Retrieving LCM information from $Computer"
                $LCMConfig = Get-DscLocalConfigurationManager -CimSession $Computer -ErrorAction Stop
            }
            catch {
                Write-Error -Message "An error has occurred. Error details: $_.Exception.Message"
                continue
            }        
            
            $servermof = "$ConfigurationPath\$Computer.mof"

            if (-not(Get-ChildItem -Path $servermof -ErrorAction SilentlyContinue)) {
                Write-Error -Message "Unable to find MOF file for $Computer in location: $ConfigurationPath"
            } 
            elseif ($LCMConfig.RefreshMode -ne 'Pull') {
                Write-Error -Message "The LCM on $Computer is not configured for DSC pull mode."
            }
            elseif ($LCMConfig.DownloadManagerName -ne 'DscFileDownloadManager' -and $LCMConfig.ConfigurationDownloadManagers.ResourceId -notlike '`[ConfigurationRepositoryShare`]*') {
                Write-Error -Message "LCM on $Computer not configured to receive configuration from DSC SMB pull server"
            }
            elseif (-not($LCMConfig.ConfigurationID)) {
                Write-Error -Message "A ConfigurationID (GUID) has not been set in the LCM on $Computer"
            }
            else {
                if ($LCMConfig.ConfigurationDownloadManagers.SourcePath) {
                    $SMBPath = "$($LCMConfig.ConfigurationDownloadManagers.SourcePath)"
                }
                elseif ($LCMConfig.DownloadManagerCustomData.Value) {
                    $SMBPath = "$($LCMConfig.DownloadManagerCustomData.Value)"
                }

                Write-Verbose -Message "Creating DSCChecksum for $servermof"
                New-DSCCheckSum -ConfigurationPath $servermof -Force

                if (Test-Path -Path $SMBPath) {

                    $guidmof = Join-Path -Path $SMBPath -ChildPath "$($LCMConfig.ConfigurationID).mof"

                    try {
                        Write-Verbose -Message "Copying $servermof.checksum to $guidmof.checksum"
                        Copy-Item -Path "$servermof.checksum" -Destination "$guidmof.checksum" -ErrorAction Stop

                        Write-Verbose -Message "Copying $servermof to $guidmof"
                        Copy-Item -Path $servermof -Destination $guidmof -ErrorAction Stop
                    }
                    catch {
                        Write-Error -Message "An error has occurred. Error details: $_.Exception.Message"                    
                    }

                }
                else {
                    Write-Error -Message "Unable to connect to $SMBPath as specified in the LCM on $Computer for it's DSC pull server"
                }
            }
        }
    }
}