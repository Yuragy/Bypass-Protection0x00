#Requires -Version 4.0
function New-MrGuid {

<#
.SYNOPSIS
    Creates a new GUID (globally unique identifier).
 
.DESCRIPTION
    The New-MrGuid function creates a new globally unique identifier which is a 128-bit integer number.

.EXAMPLE
     New-MrGuid
 
.INPUTS
    None
 
.OUTPUTS
    System.Guid
 
.NOTES
    Author:  Mike F Robbins
    Website: http://mikefrobbins.com
    Twitter: @mikefrobbins
#>

    [Guid]::NewGuid()

}