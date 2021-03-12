# vcpkg port updater

A really crappy port updater for easing the pain of updating a port in a private git registry. This is only meant for my personal use, so it would possibly not work on your machine. Use at your own risk.

Also I believe that after the registries feature of vcpkg is stablized, there'll be an official method (maybe a vcpkg sub-command) for updating private ports, so this one is really just a temporary hack.

Please add `temp/` to your ports repo's `.gitignore` list if you want to use this, since this tool creates that directory for testing hash and cloning the remote repo.
