import os

import tarfile
import subprocess
import Utils


class VulkanConfiguration:
    requiredVulkanVersion = "1.2.170.0"
    vulkanDirectory = "./3rdParty/VulkanSDK"

    @classmethod
    def Validate(cls):
        if not cls.CheckVulkanSDK():
            print("Vulkan SDK not installed correctly.")
            return

    @classmethod
    def CheckVulkanSDK(cls):
        vulkanSDK = os.environ.get("VULKAN_SDK")
        if vulkanSDK is None:
            print("\nYou don't have the Vulkan SDK installed!")
            cls.__InstallVulkanSDK()
            return False
        else:
            print("\nLocated Vulkan SDK at {vulkanSDK}")

        if cls.requiredVulkanVersion not in vulkanSDK:
            print(f"You don't have the correct Vulkan SDK version! (Engine requires {cls.requiredVulkanVersion})")
            cls.__InstallVulkanSDK()
            return False

        print(f"Correct Vulkan SDK located at {vulkanSDK}")
        return True

    @classmethod
    def __InstallVulkanSDK(cls):
        permissionGranted = False
        while not permissionGranted:
            reply = str(input("Would you like to install VulkanSDK {0:s}? [Y/N]: "
                              .format(cls.requiredVulkanVersion))).lower().strip()[:1]
            if reply == 'n':
                return
            permissionGranted = (reply == 'y')

        vulkanInstallURL = f"https://sdk.lunarg.com/sdk/download/{cls.requiredVulkanVersion}/linux/vulkansdk-linux-x86_64-1.2.170.0.tar.gz"
        vulkanPath = f"{cls.vulkanDirectory}/vulkansdk-linux-x86_64-{cls.requiredVulkanVersion}.tar.gz"
        print("Downloading {0:s} to {1:s}".format(vulkanInstallURL, vulkanPath))
        Utils.DownloadFile(vulkanInstallURL, vulkanPath)
        print("Extracting Vulkan SDK tarball...")

        Utils.UnPackingTarball(os.path.abspath(vulkanPath), cls.vulkanDirectory)
        subprocess.call(['source' f'{cls.vulkanDirectory}/1.2.170.0/setup-env.sh'])

        print("Re-run this script after installation!")
        quit()


if __name__ == "__main__":
    VulkanConfiguration.Validate()
