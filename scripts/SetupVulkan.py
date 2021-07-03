import os

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
        vulkanPath = f"{cls.vulkanDirectory}/VulkanSDK-{cls.requiredVulkanVersion}.tar.gz"
        print("Downloading {0:s} to {1:s}".format(vulkanInstallURL, vulkanPath))
        # Utils.DownloadFile(vulkanInstallURL, vulkanPath)
        print("Running Vulkan SDK installer...")

        process = subprocess.Popen(['tar', 'xvf', os.path.abspath(vulkanPath), '-C', cls.vulkanDirectory],
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        # TODO: linux installation

        print("Re-run this script after installation!")
        quit()


if __name__ == "__main__":
    VulkanConfiguration.Validate()
