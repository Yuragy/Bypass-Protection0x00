#Requires -Version 4.0
function Publish-MrDSCResourceToSMB {

<#
.SYNOPSIS
    Publishes the module(s) of the specified DSC Resource(s) to an SMB based DSC pull server.
 
.DESCRIPTION
    Publish-MrDSCResourceToSMB is an advanced PowerShell function that publishes one or more DSC resource
    modules to an SMB based DSC server pull server.
 
.PARAMETER Name
    The name of the DSC resource. This is not necessarily the same as the root module containing the DSC
    resource.

.PARAMETER Module
    The name of the module containing the DSC resource.

.PARAMETER SMBPath
    The UNC path of the SMB share used as the DSC pull server for DSC Resource distribution.
 
.EXAMPLE
     Publish-MrDSCResourceToSMB -Name xSMBShare, xFirewall -SMBPath \\Server01\Share

.EXAMPLE
     'xSMBShare', 'xFirewall' | Publish-MrDSCResourceToSMB -SMBPath \\Server01\Share
 
.INPUTS
    String
 
.OUTPUTS
    None
 
.NOTES
    Author:  Mike F Robbins
    Website: http://mikefrobbins.com
    Twitter: @mikefrobbins
#>

    [CmdletBinding(DefaultParameterSetName='Name')]
    param (

        [Parameter(Mandatory,
                   ValueFromPipeline,
                   ParameterSetName='Name')]
        [ValidateScript({
            If (Get-DscResource -Name $_) {
                $True
            }
            else {
                Throw "$_ is not a valid DSC resource name or was not found on $env:COMPUTERNAME."
            }
        })]
        [string[]]$Name,

        [Parameter(Mandatory,
                   ValueFromPipelineByPropertyName,
                   ParameterSetName='Module')]
        [ValidateScript({
            If (Get-DscResource -Module $_) {
                $True
            }
            else {
                Throw "$_ is not a valid DSC resource module or was not found on $env:COMPUTERNAME."
            }
        })]
        [string[]]$Module,

        [Parameter(Mandatory)]
        [ValidateScript({Test-Path -Path $_ -PathType Container})]
        [string]$SMBPath

    )

    PROCESS {
        $Params = @{}

        if ($PSBoundParameters.Name){
            $Params.Name = $Name
        }
        else {
           $Params.Module = $Module
        }
        
        $DSCResources = Get-MrDSCResourceModulePath @Params
                
        foreach ($DSCResource in $DSCResources) {

            $Guid = New-MrGuid                   

            New-MrZipFile -Directory "$($DSCResource.ModulePath)" -FileName "$($SMBPath)\$($DSCResource.Module)_$($DSCResource.ModuleVersion).zip" -Force
            New-DSCCheckSum -ConfigurationPath "$($SMBPath)\$($DSCResource.Module)_$($DSCResource.ModuleVersion).zip" -OutPath "$SMBPath" -Force

        }

    }

}