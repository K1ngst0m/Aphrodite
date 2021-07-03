import os
import subprocess
import platform

from SetupPython import PythonConfiguration as PythonRequirements
from SetupVulkan import VulkanConfiguration as VulkanRequirements

# Make sure everything we need for the setup is installed
PythonRequirements.Validate()

os.chdir('./../')  # Change from devtools/scripts directory to root

VulkanRequirements.Validate()

if platform.system() == "Linux":
    print("\nRunning cmake project...")
    subprocess.call([os.path.abspath("./scripts/gen_project.sh"), "nopause"])

print("\nSetup completed!")
