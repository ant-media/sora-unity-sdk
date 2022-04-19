#if UNITY_IOS

using UnityEngine;
using UnityEditor;
using UnityEditor.iOS.Xcode;
using UnityEditor.Callbacks;

public class SoraUnitySdkPostProcessor
{
    [PostProcessBuildAttribute(500)]
    public static void OnPostprocessBuild(BuildTarget buildTarget, string pathToBuiltProject)
    {
        if (buildTarget != BuildTarget.iOS)
        {
            return;
        }

        var projPath = pathToBuiltProject + "/Unity-iPhone.xcodeproj/project.pbxproj";
        PBXProject proj = new PBXProject();
        proj.ReadFromFile(projPath);
#if UNITY_2019_3_OR_NEWER
        string guid = proj.GetUnityFrameworkTargetGuid();
#else
        string guid = proj.TargetGuidByName("Unity-iPhone");
#endif

        proj.AddBuildProperty(guid, "OTHER_LDFLAGS", "-ObjC");
        proj.AddFrameworkToProject(guid, "VideoToolbox.framework", false);
        proj.AddFrameworkToProject(guid, "GLKit.framework", false);
        proj.AddFrameworkToProject(guid, "Network.framework", false);
        // libwebrtc.a contains the new libvpx, libiPhone-lib.a contains the old libvpx,
        // The default link order will use the old libvpx.
        // To avoid that, delete libiPhone-lib.a and add a new one.
        // Change the link order.
        string fileGuid = proj.FindFileGuidByProjectPath("Libraries/libiPhone-lib.a");
        proj.RemoveFileFromBuild(guid, fileGuid);
        proj.AddFileToBuild(guid, fileGuid);

        proj.WriteToFile(projPath);
    }
}

#endif
