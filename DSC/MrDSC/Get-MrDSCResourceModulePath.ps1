#Requires -Version 4.0
function Get-MrDSCResourceModulePath {

<#
.SYNOPSIS
    Returns path and version information for the root module of the specified DSC resource.
 
.DESCRIPTION
    The Get-MrDSCResourceModulePath function returns the name, module name, module version,
    and module path for the root module of the specified DSC resource.
 
.PARAMETER Name
    DSC resource name.

.PARAMETER Module
    The name of the module containing the DSC resource.

.EXAMPLE
     Get-MrDSCResourceModulePath -Name Archive

.EXAMPLE
     Get-MrDSCResourceModulePath -Name xNetworking
 
.INPUTS
    None
 
.OUTPUTS
    PSCustomObject
 
.NOTES
    Author:  Mike F Robbins
    Website: http://mikefrobbins.com
    Twitter: @mikefrobbins
#>

    [CmdletBinding(DefaultParameterSetName='Name')]
    param (
        [Parameter(Mandatory,
                   ParameterSetName='Name')]
        [string[]]$Name,

        [Parameter(Mandatory,
                   ParameterSetName='Module')]
        [string[]]$Module
    )

    $Params = @{}
    
    if ($PSBoundParameters.Name){
        $DSCResources = Get-DscResource -Name $Name
    }
    else {
        $DSCResources += foreach ($M in $Module) {
            Get-DscResource -Module $M
        }
    }
    
    foreach ($DSCResource in $DSCResources) {
        
        try {
            $ModuleInfo = Get-Module -Name $DSCResource.Module -ListAvailable -ErrorAction Stop
        }
        catch {
            Write-Warning -Message "The '$($DSCResource.Name)' DSCResource does not have a module specified. Error details: $_"
        }        

        [pscustomobject]@{
            Name = $DSCResource.Name
            Module = $DSCResource.Module
            ModuleVersion = $ModuleInfo.Version
            ModulePath = $ModuleInfo.ModuleBase
        }
    }
}
