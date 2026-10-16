# iOS device signing setup

The regular iOS build workflow deliberately creates an unsigned device build. This optional workflow exports a development-signed IPA only after the repository owner supplies valid Apple signing material as GitHub Actions secrets.

## Before you start

- An Apple Developer team/account able to create an Apple Development certificate and an iOS App Development provisioning profile.
- A provisioning profile for bundle ID com.retroshelf.originalps2 that includes the iPhone/iPad devices on which you intend to install the app.
- An Apple Development signing certificate exported as a password-protected .p12 file, plus the matching provisioning profile .mobileprovision.
- A trusted way to export the certificate and private key. Do not upload private keys to public issues, commit them to the repository, or send them in chat.

A free Apple Personal Team may have additional restrictions and shorter-lived profiles. The workflow does not bypass Apple's signing or device-registration requirements.

## Add repository Actions secrets

On GitHub, open the repository, then go to **Settings → Secrets and variables → Actions → New repository secret**. Add these exact names:

| Secret name | Value |
| --- | --- |
| APPLE_TEAM_ID | The 10-character Team ID shown in your Apple Developer account |
| IOS_CERTIFICATE_P12_BASE64 | Base64 text encoding of the password-protected Apple Development .p12 file |
| IOS_CERTIFICATE_PASSWORD | Password used to export the .p12 file |
| IOS_PROVISIONING_PROFILE_BASE64 | Base64 text encoding of the matching .mobileprovision profile |
| IOS_PROVISIONING_PROFILE_NAME | The profile's exact name as shown in the Apple Developer portal |
| IOS_KEYCHAIN_PASSWORD | A new, strong, temporary password used only by the CI runner's temporary keychain |

The .p12 and .mobileprovision files must be converted to base64 before pasting their contents into GitHub Secrets. Keep the source files and encoded text private. Do not put any secret values in workflow YAML, issues, pull-request comments, or commit history.

## Run the signed build

1. Merge the optional signing workflow only after reviewing it.
2. Open **Actions → Signed iOS development build → Run workflow**.
3. Select main and start the workflow.
4. If the signing assets are valid and the iPhone is included in the provisioning profile, download the OriginalPS2-iOS-Development-IPA artifact from the successful run.

This is a development IPA, not an App Store or TestFlight release. A signed IPA is not guaranteed to install unless its provisioning profile is valid for the target device. The emulator's PS2 boot and gameplay systems are still incomplete.
