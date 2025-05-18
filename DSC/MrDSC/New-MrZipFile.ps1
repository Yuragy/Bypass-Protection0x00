#Requires -Version 4.0
function New-MrZipFile {

<#
.SYNOPSIS
    Creates a new zip file or archive from the files in the specified folder.
 
.DESCRIPTION
    The New-MrZipFile function creates a new zip (or compressed) archive file from the files in the
    specified folder. An archive file allows multiple files to be packaged, and optionally compressed,
    into a single zipped file for easier distribution and storage.
 
.PARAMETER Directory
    Specifies the path to the folder that you want to add to the archive zipped file.

.PARAMETER FileName
    Specifies the path to the archive output file. The specified FileName value should include the desired
    name of the output zipped file; it specifies either the absolute or relative path to the zipped file.

.PARAMETER Force
    Forces an existing zip or archive file to be overwritten.

.EXAMPLE
     New-MrZipFile -Directory 'C:\FolderToZip -FileName '\\Server01\SMBShare\MyArchive.zip' -Force
 
.INPUTS
    None
 
.OUTPUTS
    System.IO.FileInfo
 
.NOTES
    Author:  Mike F Robbins
    Website: http://mikefrobbins.com
    Twitter: @mikefrobbins
#>

    [CmdletBinding()]
    param (
        [ValidateScript({
          if (Test-Path -Path $_ -PathType Container) {
            $True
          }
          else {
            Throw "'$_' is not a valid directory."
          }
        })]
        [string]$Directory,

        [ValidateScript({
          if (Test-Path -Path (Split-Path -Parent $_ -OutVariable Parent) -PathType Container) {
            if ((Resolve-Path -Path $Parent).Path -eq (Resolve-Path -Path $Directory).Path) {
                Throw "'$(Split-Path -Leaf $_)' cannot be placed in the '$Directory' folder because it's the folder being zipped."
            }
            $True
          }
          else {
            Throw "'$Parent' is not a valid directory."
          }
        })]
        [string]$FileName,

        [switch]$Force
    )

    $Directory = Resolve-Path -Path $Directory
    Write-Verbose -Message "Directory changed to the fully qualified path: '$Directory'"

    $FileName = Join-Path -Path (Resolve-Path -Path (Split-Path -Parent $FileName)).ProviderPath -ChildPath (Split-Path -Leaf $FileName)
    Write-Verbose -Message "FileName changed to the fully qualified path: '$FileName'"

    if ($PSBoundParameters.Force -and (Test-Path -Path $FileName)) {
        Write-Verbose -Message "Attempting to remove existing file '$FileName'"
        Remove-Item -Path $FileName -Force -Confirm:$false
    }

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [System.IO.Compression.ZipFile]::CreateFromDirectory($Directory, $FileName, 'fastest', $false)

}