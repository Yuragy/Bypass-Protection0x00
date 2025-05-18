configuration ConfigureHyperVHost {

    Import-DscResource -ModuleName PSDesiredStateConfiguration, cMrComputerName, cMrDomainName, StackExchangeResources, cMrRDP, xNetworking, cJumboFrames, xHyper-V 

    node $AllNodes.NodeName {

        cMrComputerName ComputerName {
            ComputerName = $Node.NodeName
        }

        cMrDomainName DomainName {
          DomainName = $Node.DomainName
        }

        Timezone Central_Standard_Time {
            Name = $node.TimeZone
            Ensure = 'Present'
        }

        cMrRDP RDP {
            UserAuthentication = $Node.RDP
            Ensure = 'Present'
        }

        $Node.WindowsFeature.ForEach({
            WindowsFeature $_  {
                Name = $_
                Ensure = 'Present'
            }
        })

        $Node.Service.ForEach({
            Service $_ {
                Name = $_
                StartupType = 'Automatic'
                State = 'Running'
            }
        })

    }
    
    node $AllNodes.Where({$_.Role -eq 'HyperVHost'}).NodeName {

        $Node.VMSwitch.ForEach({
            xVMSwitch $_.ID {  
                Name = $_.Name
                Type = 'External' 
                NetAdapterName = $_.Adapter
                Ensure = 'Present'
                DependsOn = '[WindowsFeature]Hyper-V' 
            }
        })

        $Node.NIC.ForEach({        
            xIPAddress $_.ID {
                IPAddress = $_.IP
                InterfaceAlias = $_.Adapter
                DefaultGateway = $_.Gateway
                SubnetMask = $_.SubnetMask
                AddressFamily = $_.Family
                DependsOn = $_.DependsOn
            }

        })

        $Node.NIC.Where({$_.Adapter -notlike '*iSCSI Network*'}).ForEach({
            xDNSServerAddress $_.ID {
                Address = $_.DNS
                InterfaceAlias = $_.Adapter
                AddressFamily = $_.Family
            }
        })

        $Node.NIC.Where({$_.Adapter -like '*iSCSI Network*'}).ForEach({
            cJumboFrames $_.ID {
                InterfaceAlias = $_.Adapter
                Ensure = 'Present'
                DependsOn = $_.JFDependsOn
            }
        })

    }

}
