set -e # cancel script on error

SDK_VERSION="1.4.357.1"

echo "Installing Vulkan SDK version ${SDK_VERSION}"

SDK_URL="https://sdk.lunarg.com/sdk/download/${SDK_VERSION}/linux/vulkansdk-linux-x86_64-${SDK_VERSION}.tar.xz"

echo "Install URL: ${SDK_URL}"

INSTALL_DIR="$PWD/VulkanSDK"

echo "Vulkan SDK will be installed to ${INSTALL_DIR}"

ARCHIVE="/tmp/vulkansdk-${SDK_VERSION}.tar.xz"

echo "==> Downloading Vulkan SDK ${SDK_VERSION}..."
curl -fL "$SDK_URL" -o "$ARCHIVE"

echo "==> Extracting Vulkan SDK..."
mkdir -p "$INSTALL_DIR"
tar -xJf "$ARCHIVE" -C "$INSTALL_DIR"

echo "==> Cleaning up archive..."
rm -f "$ARCHIVE"

echo "Vulkan SDK extracted to:"
find "$INSTALL_DIR" -maxdepth 2 -type d -name "*${SDK_VERSION}*" -print

echo "Add 'source ${INSTALL_DIR}/${SDK_VERSION}/setup-env.sh' to your .bashrc"  
