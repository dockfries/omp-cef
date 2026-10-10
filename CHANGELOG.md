
## [1.4.3](https://github.com/dockfries/omp-cef/compare/v1.4.2..v1.4.3) (2026-10-10)

### 🧹 Chore

- *(client)* Trace the create/destroy path to locate the cefspam crash - ([0b492ed](https://github.com/dockfries/omp-cef/commit/0b492eda45fc4d9f7779db174a6d2125085b9e48))
- *(client,ci)* Make the network bookkeeping atomic and build on pushes to main - ([9b58d7c](https://github.com/dockfries/omp-cef/commit/9b58d7c09ccaa14825855913e5b593d6dce1a75e))
- Bump the version to 1.4.3 - ([32b6398](https://github.com/dockfries/omp-cef/commit/32b6398a74827460ec8a304f50cec25acdceddaa))

### 🐛 Bug Fixes

- *(client)* Stop hanging the exit on the io and audio threads - ([e34cb78](https://github.com/dockfries/omp-cef/commit/e34cb78901494e76191534edd0ab0c0ef204c1ac))
- *(client)* Make the network bookkeeping atomic - ([4c7474f](https://github.com/dockfries/omp-cef/commit/4c7474f9fbf3907a3965afceaa4868b518ce0b6f))
- *(client)* Send UDP from the thread that owns the socket - ([3e54ff3](https://github.com/dockfries/omp-cef/commit/3e54ff3fe74914c88bc66a3667f8f431c85c2096))
- *(client)* Give the resource gate a deadline and a way to report failure - ([089cc42](https://github.com/dockfries/omp-cef/commit/089cc42ce476b215645f7b2b2c61ccf0ab0aac3a))
- *(client)* Verify the class selection pointer before patching it - ([d8355a5](https://github.com/dockfries/omp-cef/commit/d8355a5a5dfe75a4842cff4058aff3866e00b4a8))
- *(client)* Report a bad master resource key instead of waiting forever - ([a3e5753](https://github.com/dockfries/omp-cef/commit/a3e575364657c92df87fcec91c8f8f8bb5b53423))
- *(client)* Drive the device reset paths from the snapshot and drop the diagnostics - ([7c3d58e](https://github.com/dockfries/omp-cef/commit/7c3d58ea42e50923fdb97bd10017a91b3a721c77))
- *(client)* Swap object textures from the bindings in the snapshot - ([0601c53](https://github.com/dockfries/omp-cef/commit/0601c5349e5baa44b7bc53d80d473df5659c3d9a))
- *(client)* Own the paint buffer and the world renderer in the browser instance - ([8fb0c0f](https://github.com/dockfries/omp-cef/commit/8fb0c0fe00f9801763c1f62fd14aa843eb95e0ef))
- *(client)* Let RenderAll walk an immutable browser list - ([4bc2be4](https://github.com/dockfries/omp-cef/commit/4bc2be41760163970974b29417ed009ff5c4e41b))
- *(client)* Keep the window subclass, the renderer teardown and the object attach in order - ([5488004](https://github.com/dockfries/omp-cef/commit/54880040688f6b128e1bbac17e1bb3ee36f1549c))
- *(client,server)* Harden the browser lifetime, the network path and the exit - ([643a203](https://github.com/dockfries/omp-cef/commit/643a203a3b34b3bfb161688741f838f948ba68be))

### ♻️ Refactoring

- *(client)* Remove the two maps nothing writes any more - ([0fbe134](https://github.com/dockfries/omp-cef/commit/0fbe1340ed18c66498c4cc52966a4325b060e4ae))
- *(client)* Publish the world renderer together with each browser in the snapshot - ([b2d191e](https://github.com/dockfries/omp-cef/commit/b2d191e01eb41d17a9edc240f5e1688e19df6224))

### Merge

- Browser state race fix, resource gate deadlines and network single owner - ([35b8652](https://github.com/dockfries/omp-cef/commit/35b8652c060003627fbf8fdb3c3d8da4868037fa))




## [1.4.2](https://github.com/dockfries/omp-cef/compare/v1.4.1..v1.4.2) (2026-10-09)

### 🧹 Chore

- Reset the changelog for the 1.4.2 rebuild - ([98163bc](https://github.com/dockfries/omp-cef/commit/98163bc994124fe5c314ff338cdade8f78152373))
- Drop the UTF-8 BOM from the remaining sources - ([33a0610](https://github.com/dockfries/omp-cef/commit/33a0610ead35e94c61eb9632d15aa0ee9bb1705f))
- Replace the remaining non-ASCII characters in sources - ([f8fe81f](https://github.com/dockfries/omp-cef/commit/f8fe81fe2f23bfeb796a802c4c93cb7b18d3e46b))
- Bump version to 1.4.2 - ([673b5b7](https://github.com/dockfries/omp-cef/commit/673b5b7f6ed0b485e38d707ac27be8a429261b6e))

### ✨ Features

- Add browser layer control - ([566f129](https://github.com/dockfries/omp-cef/commit/566f129528efbe51013bb532fc2b6e3a29edbc7c))

### 🐛 Bug Fixes

- *(ci)* Point generated changelog links at the repository that publishes the releases - ([fa792be](https://github.com/dockfries/omp-cef/commit/fa792be0923815fa4e673abb31c669e07b80cefb))
- *(client)* Report browser creation failures to the server - ([2029147](https://github.com/dockfries/omp-cef/commit/20291476f6fc9d484ad4d99ccf64f5005abbdce9))
- *(client)* Close browsers through CEF's lifecycle and shut CEF down in order - ([c9cf0a7](https://github.com/dockfries/omp-cef/commit/c9cf0a7e7b09ce9a10a18201920fae7cc75515de))
- *(client)* Harden SA-MP version detection - ([760fe61](https://github.com/dockfries/omp-cef/commit/760fe614bf39c583778cc47f8697352651df5698))
- *(client)* Move cross-thread game state onto the game thread - ([3e33e38](https://github.com/dockfries/omp-cef/commit/3e33e3801cc8c64addf5cb9f62fdc4a13be553fe))
- *(client)* Detect SA-MP builds by PE signature before the version resource - ([ff70ba0](https://github.com/dockfries/omp-cef/commit/ff70ba0b50990dd8ccaf074f4faa00b49e9eb257))
- *(omp)* Release string arguments of registered event callbacks - ([0501e53](https://github.com/dockfries/omp-cef/commit/0501e531bdd57acde926f5445cece3191cf714e8))
- *(samp)* Stop running download and key callbacks after a failed push - ([a36ef36](https://github.com/dockfries/omp-cef/commit/a36ef366897ec4d8702c07b10ea805ba0f4d453e))
- *(samp)* Stop rewinding the AMX heap when a string argument cannot be pushed - ([7f0a3a5](https://github.com/dockfries/omp-cef/commit/7f0a3a5a9e56ab4941b8ff0733ad397043534cd3))
- *(server)* Dispatch a registered event with a copy of the callback name - ([9a5747f](https://github.com/dockfries/omp-cef/commit/9a5747f565ec7d6eec8bbfac1cd2e032c419eddc))
- *(server)* Invalidate the Pawn bridge, surface startup failures, dedupe file requests - ([33cc5a4](https://github.com/dockfries/omp-cef/commit/33cc5a4417eb236fda5ee72457a7ec9d4f0143ef))
- *(server)* Validate cookie and client public key lengths - ([e93edd4](https://github.com/dockfries/omp-cef/commit/e93edd461468b5b74cab1b72bf4212a74ee54aee))
- *(server)* Run the network loop on the main thread - ([418992e](https://github.com/dockfries/omp-cef/commit/418992eee406a467ab03135f84e166c43db8db6b))
- Prepare fork release builds and sampctl resources - ([ac5c64f](https://github.com/dockfries/omp-cef/commit/ac5c64feda3af2455f282881a32297b10a001264))

### 🤖 CI

- Fail the release when the embedded version does not match the requested one - ([3f5d22f](https://github.com/dockfries/omp-cef/commit/3f5d22f764c48d39ec87abad74b5379327954bd7))

### 📦 Build

- Compile MSVC sources and narrow literals as UTF-8 - ([5ec39ff](https://github.com/dockfries/omp-cef/commit/5ec39ff3c077ae03afce936ef7df57d7542d13fd))
- Update CMakePresets.json - ([69041a2](https://github.com/dockfries/omp-cef/commit/69041a26b3d5572aed690dcc179d99cf54419547))

## New Contributors ❤️

* @wigarddev made their first contribution
* @Masvidal99 made their first contribution



## [1.4.1](https://github.com/dockfries/omp-cef/compare/v1.4.0..v1.4.1) (2026-09-09)

### 📖 Documentation

- Add zh-CN README and clean changelog - ([5814d78](https://github.com/dockfries/omp-cef/commit/5814d7823549648fcc54e9b71aac2d312b69ddff))
- Fix duplicate changelog entries - ([ecc1d9c](https://github.com/dockfries/omp-cef/commit/ecc1d9c32dfc8233c82c8c88784b2f1e555fa9c9))

### 🐛 Bug Fixes

- *(client)* Address for 037r5 - ([ca6571c](https://github.com/dockfries/omp-cef/commit/ca6571c58286899d7c3043efa4c5185efcb540d7))

### 🤖 CI

- Build and publish cef-samp for x86 and x64 - ([24da1de](https://github.com/dockfries/omp-cef/commit/24da1defb29535f47bcf6ea6c24338c0bcad19db))

### 📦 Build

- *(sampgdk)* Update to 5.0.0 with fixes - ([cca43aa](https://github.com/dockfries/omp-cef/commit/cca43aa4668be7eb107e00bcbe3c70e7bb973e80))
- *(sampgdk)* Add x64 support - ([38226b5](https://github.com/dockfries/omp-cef/commit/38226b549762ef9390d4621c6bcfd0328d3e7912))
- Consume sampgdk as a git submodule - ([fdf97df](https://github.com/dockfries/omp-cef/commit/fdf97df54f06b97fd32f2914f27191c7f62706c3))



## [1.4.0](https://github.com/dockfries/omp-cef/compare/v1.3.0..v1.4.0) (2026-08-10)

### 🧹 Chore

- Update CHANGELOG for v1.4.0 - ([692db89](https://github.com/dockfries/omp-cef/commit/692db899a2ceb70be0eb1a47d49e775d6c98c88d))

### ✨ Features

- *(client)* Expose game screen capture to CEF - ([c7056eb](https://github.com/dockfries/omp-cef/commit/c7056eb28de7722a5383990edf0d07eebf1b9507))

### 🐛 Bug Fixes

- *(client)* Restore static CEF textures after Alt+Tab - ([6a7412a](https://github.com/dockfries/omp-cef/commit/6a7412ad0caf6236e5a3058515927fb1f3b131c8))


## [1.3.0](https://github.com/aurora-mp/omp-cef/compare/v1.2.0..v1.3.0) (2026-06-19)

### 🧹 Chore

- *(client)* Some improvements/fixes #17 - ([7a95408](https://github.com/aurora-mp/omp-cef/commit/7a95408f4e781ae3d34d4ecb627ff78c9d5bb3f0))
- *(runtime)* Disable draw if game is paused - ([ce6e4bd](https://github.com/aurora-mp/omp-cef/commit/ce6e4bd641ae032861c41a22894788b894917ca6))
- Update CHANGELOG for v1.3.0 - ([4f077c4](https://github.com/aurora-mp/omp-cef/commit/4f077c4b8850f591aa5005a34adb25a685ab0643))
- Update CHANGELOG for v1.3.0 - ([4292d35](https://github.com/aurora-mp/omp-cef/commit/4292d35ecba35e49c0427b9eb8cc6b78fedccd16))
- Update .gitignore - ([6dec55f](https://github.com/aurora-mp/omp-cef/commit/6dec55f9d68823e8348ee28944b8ed4866b68e23))
- Bump version to 1.3.0 - ([7c29e0f](https://github.com/aurora-mp/omp-cef/commit/7c29e0f2d52ce8743860b27a96ee061cf128c41b))

### ✨ Features

- *(server)* Add resource loader UI mode & some improvements - ([a97da20](https://github.com/aurora-mp/omp-cef/commit/a97da20fdb9b430d8fcfa982ffb3c14aacef81a9))
- Add custom player list mode with samp scoreboard hook (CEF_SetPlayerListMode) - ([42b854f](https://github.com/aurora-mp/omp-cef/commit/42b854ff77fb9aa3a40f8ea325fba3e7f00b6891))
- Add CEF_LoadUrl (missing native needed to navigate an existing browser) - ([f87bc82](https://github.com/aurora-mp/omp-cef/commit/f87bc824df648991096a503ec4c7f866416d306a))
- [**breaking**] Update CEF to the latest version - ([0e98977](https://github.com/aurora-mp/omp-cef/commit/0e98977355c491f2a7a75c9fbf8f3a4d8cede145))
- Add custom escape menu mode (native CEF_SetEscapeMenuMode) - ([5d13e14](https://github.com/aurora-mp/omp-cef/commit/5d13e14753ea8da6d8021a4e5d8c1c941675bcea))

### 🐛 Bug Fixes

- *(client)* Clear browser texture on navigation to prevent ghosting - ([d54725d](https://github.com/aurora-mp/omp-cef/commit/d54725d0d36409cda3ca9707c2e5e5a6985c52e6))
- *(client)* Ensure menu is draw - ([dd478b7](https://github.com/aurora-mp/omp-cef/commit/dd478b7eeff9fd634cab59d5904fa042805a6edb))
- *(client)* CEF browsers is now hidden in other menus - ([90116ff](https://github.com/aurora-mp/omp-cef/commit/90116ffa02560d4ce889ccb64907b4c5f4b48004))
- *(client)* Allow overlay browser resolutions above 1440p #25 - ([d211550](https://github.com/aurora-mp/omp-cef/commit/d211550849cc7dde651d71cd914a7340bf2f8900))
- WorldObject3D texture swaps for hidden browsers - ([fd5e81d](https://github.com/aurora-mp/omp-cef/commit/fd5e81d10ac582401281d136ebf0775335d9fced))
- Handle CEF reconnects after server restarts #31 - ([52c118c](https://github.com/aurora-mp/omp-cef/commit/52c118cc775472185b6140498cfa9f4fcdad68ff))
- Cef thread races on browser creation (D3D) - ([06fb567](https://github.com/aurora-mp/omp-cef/commit/06fb567f58b9e497dfe6b63a133263c4f157166f))

### 🤖 CI

- Fix release server artifact packaging - ([f0548f4](https://github.com/aurora-mp/omp-cef/commit/f0548f462ea525d4bf61c9542da9ba6a909d6789))
- Fix cmake (glm issue cause omp-sdk to updated..) - ([17b13d4](https://github.com/aurora-mp/omp-cef/commit/17b13d44e7f2bbc2fb71b1aec532ac533a33f63e))

### ✅ Testing

- *(samp)* Update server.cfg - ([c9aa89f](https://github.com/aurora-mp/omp-cef/commit/c9aa89fe14c758b93da89acecfdc0f989b265c78))
- Update README - ([3fe314a](https://github.com/aurora-mp/omp-cef/commit/3fe314ad5d81a1e44fb65b451fc1684199d0b929))
- Update sample - ([ad54250](https://github.com/aurora-mp/omp-cef/commit/ad5425059ab2d33cab907524c76568ebc0c65ab3))
- Add demo gamemode - ([7465bda](https://github.com/aurora-mp/omp-cef/commit/7465bda34c6baa509cd7f24c0caf292b8425415b))

### ⚡ Performance

- *(client)* Sync CEF frames to render tick - ([285814b](https://github.com/aurora-mp/omp-cef/commit/285814bf84d4d96a49f8772302758bd8c26bc020))
- [**breaking**] Optimize CEF resource packing and cache validation - ([a272bb3](https://github.com/aurora-mp/omp-cef/commit/a272bb3fbcfdb5e6d4b7ebdf41d6729112f821e7))

## New Contributors ❤️

* @godperkys made their first contribution
* @bssth made their first contribution
* @ made their first contribution
* @kyro95 made their first contribution

## [1.2.0](https://github.com/aurora-mp/omp-cef/compare/v1.1.0..v1.2.0) (2026-02-19)

### 🧹 Chore

- Update CHANGELOG for v1.2.0 - ([85c12e4](https://github.com/aurora-mp/omp-cef/commit/85c12e4c7fd9f16daedb37fe9bb53253c32a825f))
- Bump next version - ([b662b38](https://github.com/aurora-mp/omp-cef/commit/b662b385cf5195c6eb588523ba85d88636165b22))

### ✨ Features

- *(client)* Add poll player stats #14 - ([4d2318f](https://github.com/aurora-mp/omp-cef/commit/4d2318f5f557a2d69df2e8279d79d98eb0f2f73e))
- *(client)* Add cef.isChatInputOpen #13 - ([37e2fc0](https://github.com/aurora-mp/omp-cef/commit/37e2fc05e348d8381f57739c6068808ba15d0747))
- *(server)* Add CEF_IsChatInputOpen & OnCefChatInputState - ([d1153bd](https://github.com/aurora-mp/omp-cef/commit/d1153bd739889ee32ecf5f217281d99209a587f4))

### 🐛 Bug Fixes

- *(client)* Fix issues with ALT + TAB & cursor restoration - ([3013b74](https://github.com/aurora-mp/omp-cef/commit/3013b7495afeba7239709c4a5449f44cfb0d444c))
- *(omp)* Fix IsPlayerNpcBot - ([eade1e4](https://github.com/aurora-mp/omp-cef/commit/eade1e47c428b367e6ae5c1534eff4e60017c610))
- Fix GLIBC compatibility - ([e68988c](https://github.com/aurora-mp/omp-cef/commit/e68988c15255100a7852b22d18101a37d3973e55))

## [1.1.0](https://github.com/aurora-mp/omp-cef/compare/v1.0.5..v1.1.0) (2026-02-12)

### 🧹 Chore

- *(client)* Improve render_manager - ([e99fbb6](https://github.com/aurora-mp/omp-cef/commit/e99fbb6b8d80b910c143fd0dadc79558e22de7c2))
- *(client)* Improve/fix plugin runtime initialization - ([0cfdf78](https://github.com/aurora-mp/omp-cef/commit/0cfdf7899d7b2bcce473055c501fc97837d09e9b))
- Update CHANGELOG for v1.1.0 - ([f84d62f](https://github.com/aurora-mp/omp-cef/commit/f84d62fc14eb683f82d9343535b7fa4803a8d489))
- Update README - ([7bee73f](https://github.com/aurora-mp/omp-cef/commit/7bee73ffad33c18afb6f6e62274da04143b7d0ba))
- Bump next version - ([97ac8a7](https://github.com/aurora-mp/omp-cef/commit/97ac8a7d1f3972d90094ad64ba2702f7cfb491e3))

### ✨ Features

- *(client)* Add `cef.exitGame` - ([c684e21](https://github.com/aurora-mp/omp-cef/commit/c684e21eefd4528d4bc1c2c764c2b302cc81eac6))
- *(client)* Add `cef.set_focus` - ([bba9a73](https://github.com/aurora-mp/omp-cef/commit/bba9a7335461d0de08eb925a5e357394252a02c6))
- Add `CEF_ToggleChatInput` - ([5bab951](https://github.com/aurora-mp/omp-cef/commit/5bab951a2a47ff92f7ba7dbebd4cbdb09d18677b))
- Add `CEF_ExitGame` #8 - ([9f510a4](https://github.com/aurora-mp/omp-cef/commit/9f510a4926a61dfa20e6145ab95262cae66bbe24))
- New features added - ([00027fe](https://github.com/aurora-mp/omp-cef/commit/00027fedaeec860139003c79038d15759f0bef4a))

### 🐛 Bug Fixes

- *(client)* Fix #11 - ([0707307](https://github.com/aurora-mp/omp-cef/commit/070730766faf81d9c1f0fb2566ae7f417d5a97c3))
- *(client)* Improve runtime, possible crash fixes (SAMP Addons) - ([429de24](https://github.com/aurora-mp/omp-cef/commit/429de24eb3bc76cd09771b5896d4eb7b2f1ac210))
- *(server)* Fix #10 - ([99870f0](https://github.com/aurora-mp/omp-cef/commit/99870f0a9b9c8c4d8ed97eb2fbc7a8ef785e6523))

## [1.0.5](https://github.com/aurora-mp/omp-cef/compare/v1.0.4..v1.0.5) (2026-02-05)

### 🧹 Chore

- Update CHANGELOG for v1.0.5 - ([6a787bf](https://github.com/aurora-mp/omp-cef/commit/6a787bfe50f57d1995019a965f429fb7ce4a79d4))

### ✨ Features

- *(server)* Extend OnCefInitialize with reason + message - ([bcf0c78](https://github.com/aurora-mp/omp-cef/commit/bcf0c78fdbd76d2139b4f55db6862f0455ff8fcf))

### 🐛 Bug Fixes

- *(client)* Make WndProc hook resilient to SA-MP replacing window proc - ([e437c69](https://github.com/aurora-mp/omp-cef/commit/e437c690e6908880e2eb06ab5df024efd51787f9))
- *(server)* Fix NotifyCefInitialize default params. - ([a414d14](https://github.com/aurora-mp/omp-cef/commit/a414d148a59481025f10c0832a74fa37d224dff6))
- Handle NPC playerid offset + add strict client/server version check + fix encoding - ([06ee8bf](https://github.com/aurora-mp/omp-cef/commit/06ee8bf185c196fc5ae59742d92c46964a88efce))

## [1.0.4](https://github.com/aurora-mp/omp-cef/compare/v1.0.3..v1.0.4) (2026-02-03)

### 🧹 Chore

- *(client)* Add version to log - ([cbe70a0](https://github.com/aurora-mp/omp-cef/commit/cbe70a0af79550094f7102cab9639a747ef4e25a))
- Update CHANGELOG for v1.0.4 - ([7d16967](https://github.com/aurora-mp/omp-cef/commit/7d1696786a6fbbc3e31f4fa985d030af496fe444))

### 🐛 Bug Fixes

- *(client)* Fix CEF OSR lag: queue OnPaint frames in CPU and upload textures on D3D thread during RenderAll - ([e0ddcfe](https://github.com/aurora-mp/omp-cef/commit/e0ddcfe421e6f9c4b48484d5c95c38f3842dfd7e))

## [1.0.3](https://github.com/aurora-mp/omp-cef/compare/v1.0.2..v1.0.3) (2026-02-03)

### 🧹 Chore

- *(client)* Normalize CefApp switches - ([cd813c7](https://github.com/aurora-mp/omp-cef/commit/cd813c7775efa77af9694618b473b246ce97fc8d))
- *(client)* Some clean up - ([04b47e1](https://github.com/aurora-mp/omp-cef/commit/04b47e1d6f5176998d6fdc023d7143533568a75f))
- *(tests)* Add very basic login example for omp & samp - ([51d7e69](https://github.com/aurora-mp/omp-cef/commit/51d7e69e9f267c325c275663018ace003d015f30))
- *(tests)* Add react webview for tests - ([cac89fa](https://github.com/aurora-mp/omp-cef/commit/cac89fa77fc10d871dad4a91ab2a08783509d14e))
- Update CHANGELOG for v1.0.3 - ([66014bc](https://github.com/aurora-mp/omp-cef/commit/66014bc4195da5a9a9c0b79d8ddae886465ee995))
- Add issue template - ([777ed37](https://github.com/aurora-mp/omp-cef/commit/777ed37bca43af6363dc966c8897e29161cbf92d))
- Update README - ([c4e61b2](https://github.com/aurora-mp/omp-cef/commit/c4e61b24e58cc0e2f06fc1a25a3d521dc3e8dc14))

### 🐛 Bug Fixes

- Fix OnCefReady callback not called when no resources need to be downloaded - ([93adc9a](https://github.com/aurora-mp/omp-cef/commit/93adc9ae9899e8613c3883eb994c30b337f28db7))

### ♻️ Refactoring

- *(client)* Replace D3D9 device proxy with vtable hooks + fallback poll - ([30c9a66](https://github.com/aurora-mp/omp-cef/commit/30c9a66ec3ede94d3b6678cf2f86b97fba0353a1))
- Replace d3ddevice9 cursor forcing with Win32 SetCursor hook - ([69218f3](https://github.com/aurora-mp/omp-cef/commit/69218f3377411dbdcd6b48176e3dcefdab260b2e))

## [1.0.2](https://github.com/aurora-mp/omp-cef/compare/v1.0.1..v1.0.2) (2026-02-01)

### 🧹 Chore

- *(client)* Clean verbose logs - ([28a1d71](https://github.com/aurora-mp/omp-cef/commit/28a1d71a5b9f72d0ffd208a2762141d3725aace4))
- Update CHANGELOG for v1.0.2 - ([2d098f7](https://github.com/aurora-mp/omp-cef/commit/2d098f743bcdea70961e09098e3d42047d2108b2))
- Update cliff (duplications) - ([d3229d4](https://github.com/aurora-mp/omp-cef/commit/d3229d40a7f7c62dfb49d935b410850730ca4db0))
- Bump next version - ([e520280](https://github.com/aurora-mp/omp-cef/commit/e5202807efb514067e96724c40ade9b9aa0f0a8a))

### ✨ Features

- *(client)* Improve DevTools, removed the DevTools shortcut (it was for easy debugging) #3 - ([a488d0d](https://github.com/aurora-mp/omp-cef/commit/a488d0ddfbbefbbc52442e5c440289c6ea2e5dc7))
- *(server)* Add new native CEF_EnableDevTools - ([6010b2d](https://github.com/aurora-mp/omp-cef/commit/6010b2daef467c3c0abd081c5453aa42e4f6879f))
- Update for #4 - ([4d4a6dd](https://github.com/aurora-mp/omp-cef/commit/4d4a6ddafc33030a1e1ccd6a48f4de6159615c91))

### 🐛 Bug Fixes

- *(client)* UI browsers not rendering (OnPresent executed after Present) - ([0e9f262](https://github.com/aurora-mp/omp-cef/commit/0e9f262df8dc481ba3c09810bd2d01cee5377adb))
- *(client)* Fix some crashes, handle device lost/reset and avoid rendering during reset , improvements #4, #6 - ([6734002](https://github.com/aurora-mp/omp-cef/commit/6734002bd5bbb9f14870221ca5876c3904bfec4f))
- *(client)* Improvements & make D3D9 hooking robust across setups (Direct3DCreate9 + CreateDevice) #6 - ([c05ccdf](https://github.com/aurora-mp/omp-cef/commit/c05ccdf713879601f10253d450b593348a214a33))

## [1.0.1](https://github.com/aurora-mp/omp-cef/compare/v1.0.0..v1.0.1) (2026-01-30)

### 🧹 Chore

- Update CHANGELOG for v1.0.1 - ([8920eec](https://github.com/aurora-mp/omp-cef/commit/8920eeca559c62939a221d2f58cac24ba4750cef))
- Bump version - ([7ee6569](https://github.com/aurora-mp/omp-cef/commit/7ee65699174583053ce099d72bd4e55c82aec6d1))
- Update README - ([f504958](https://github.com/aurora-mp/omp-cef/commit/f5049585c91a3473a3699d136b7ade0690445440))

## [1.0.0] (2026-01-30)

### 🧹 Chore

- *(client)* Clean up - ([a79ef89](https://github.com/aurora-mp/omp-cef/commit/a79ef89ed73f40239c10d207055a910f7435f44b))
- *(client)* Init from my own repository (refactoring wip) - ([3917bdb](https://github.com/aurora-mp/omp-cef/commit/3917bdbd0aec4178a8eda620cd9999aacac219d9))
- *(omp)* Clean up CMakeLists - ([4e81a83](https://github.com/aurora-mp/omp-cef/commit/4e81a83a5272381c0d7221911303deace6eab508))
- *(server)* Init from my own repository (refactoring wip) - ([b1e2fd5](https://github.com/aurora-mp/omp-cef/commit/b1e2fd51b6a79870a1a21a98826e5d4ef8d1da40))
- *(shared)* Init from my own repository - ([304f513](https://github.com/aurora-mp/omp-cef/commit/304f513dff91774ba132408d52b517f87e9a6e4f))
- Update CHANGELOG for v1.0.0 - ([894526e](https://github.com/aurora-mp/omp-cef/commit/894526e7edde3d1a53a8e986ef11eb0f56f504f5))
- Update github workflow - ([3f3612d](https://github.com/aurora-mp/omp-cef/commit/3f3612d374973fad7e3169cdabdee01b7b3c2f12))
- Update github workflows - ([4d725a2](https://github.com/aurora-mp/omp-cef/commit/4d725a2850cedad64bfc2d50b1bf1b81c89a2b59))
- Update README - ([4eee777](https://github.com/aurora-mp/omp-cef/commit/4eee777b3201ba81565f98fe496075e3d5795df5))
- Update github workflows - ([871f77a](https://github.com/aurora-mp/omp-cef/commit/871f77aa1e93bbab29d015316cb5388e19627fb2))
- Add github workflows - ([cc07512](https://github.com/aurora-mp/omp-cef/commit/cc07512dcaf7d601b41142b9453978b4617cfbd7))
- Update github actions - ([523ebb1](https://github.com/aurora-mp/omp-cef/commit/523ebb1aceac6d55f4c849968a0d7a744922cf53))
- Add github actions - ([cd3881c](https://github.com/aurora-mp/omp-cef/commit/cd3881c143872f71dde2782fabb5d698e3d8c7a4))
- Update README - ([a582246](https://github.com/aurora-mp/omp-cef/commit/a582246576f64d80851c5ac2c1355d56f750b57d))
- Update README - ([2f14c94](https://github.com/aurora-mp/omp-cef/commit/2f14c94ffc56a6bbfabe1f24511b4352d14363b4))
- Add LICENSE - ([45bc88a](https://github.com/aurora-mp/omp-cef/commit/45bc88ae4ae78f6dd9415e92968962b376ce7d61))
- Create README - ([ef40070](https://github.com/aurora-mp/omp-cef/commit/ef4007031d6a1773fd2dbb027cef1990aef1c7c1))
- Clean up - ([bfa2a2b](https://github.com/aurora-mp/omp-cef/commit/bfa2a2beb6158c8605f12afece57c95924e40510))
- Pin deps/asio to 231cb29b - ([1981601](https://github.com/aurora-mp/omp-cef/commit/19816014c76d222f7620175fcac5c5cb262ad314))
- Add submodules - ([210ba4c](https://github.com/aurora-mp/omp-cef/commit/210ba4c4e732252e9bd3899fc7dab33390f95672))
- Re init omp-sdk deps - ([95dca18](https://github.com/aurora-mp/omp-cef/commit/95dca18ad4709b1d60a9a385b095d4cbc0ca8806))
- Init repository (refactoring in progress) - ([3f46834](https://github.com/aurora-mp/omp-cef/commit/3f46834b191d6e67cabe4e3f5207acbdfd61860a))

### ✨ Features

- *(client)* Update loader progress bar color - ([ab7ec89](https://github.com/aurora-mp/omp-cef/commit/ab7ec89189d6b214a0592ffc20ffd351dedf620b))
- *(renderer)* Add cef.off - ([c22787c](https://github.com/aurora-mp/omp-cef/commit/c22787c66c7540efd9934a83c1cb9a7c3c50452f))
- *(server)* Add back samp bridge - ([f7e5ef0](https://github.com/aurora-mp/omp-cef/commit/f7e5ef0ed5a88fa56b7bf6f1c6d7f3d628eb556a))
- Added missing natives ​​(forgotten...) - ([1e53aec](https://github.com/aurora-mp/omp-cef/commit/1e53aec6667db09a2428aaca8b421db9fd24b4a9))
- Create internal browser wrapper for youtube/twitch videos - ([bacb69c](https://github.com/aurora-mp/omp-cef/commit/bacb69ceaed05985c6f52daf5ab60cb037251888))
- Add server version in log - ([60666ea](https://github.com/aurora-mp/omp-cef/commit/60666ea2f4f77caa3b98d6759e66869f47cb7380))
- Add OnCefReady callback + improvements. - ([5a93622](https://github.com/aurora-mp/omp-cef/commit/5a93622b01aff08332815c952ed66ed2ee00e547))
- Improvements + cleanup - ([2ccd3f5](https://github.com/aurora-mp/omp-cef/commit/2ccd3f5fe35e98dd534b567d61e19f3317801ba1))
- Improvements + smooth resource loader + gate browser creation until resources ready - ([158c551](https://github.com/aurora-mp/omp-cef/commit/158c55132dd656c7b66c0e99551225b5e6eb58d1))
- Add profile_dir (persist) - ([5add4e5](https://github.com/aurora-mp/omp-cef/commit/5add4e5aecd7d2569528f165608b73e080bb5a12))
- Add back RegisterEvent - ([c2c3337](https://github.com/aurora-mp/omp-cef/commit/c2c33372a1dbc171771c565bd298ae156c3e0284))
- Add back EmitBrowserEvent - ([56d2d8f](https://github.com/aurora-mp/omp-cef/commit/56d2d8ffd2042dad5b77b1d66a53ebe28436e3c6))

### 🐛 Bug Fixes

- *(renderer)* Fixes race between server emit and cef.on - ([7d4f106](https://github.com/aurora-mp/omp-cef/commit/7d4f1067bafbabd474c6202d5d508991e849f295))
- Fix github workflow for binaries in zip - ([f22e35e](https://github.com/aurora-mp/omp-cef/commit/f22e35e167727d6b87d1acd460343e02b5cc7eb2))
- Fix warnings & build - ([1f94c99](https://github.com/aurora-mp/omp-cef/commit/1f94c9959f6d62b3fc19a1dd576a9b98ae4c42b8))
- Github workflow - ([9a696db](https://github.com/aurora-mp/omp-cef/commit/9a696db8207d59a18db74403e31043a39fe8e9d3))
- Fix rc win version - ([66f61b4](https://github.com/aurora-mp/omp-cef/commit/66f61b4fff6127cb24e85f8c016bbd37d5159e3a))
- Avoid deadlock when emitting events during KCP processing - ([d97ea0d](https://github.com/aurora-mp/omp-cef/commit/d97ea0d19179859c4c91167deb5f0c2b2e474227))

## New Contributors ❤️

* @NeekoGta made their first contribution
