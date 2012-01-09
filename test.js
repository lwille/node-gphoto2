GPhoto = require("./build/Release/gphoto2");
console.log(GPhoto);
var gphoto = new GPhoto.GPhoto2();
gphoto.list(function(list) {
   console.log("Found", Object.keys(list).length, "cameras:");
   for(i = 0; i< list.length; i++) {
     cam = list[i];
     console.log("Cam:",cam);
   }
});
//     if (Object.keys(list).length > 0) {
//         console.log(list);
// 
//         id = Object.keys(list);
// 
//         console.log("Taking picture with", list[id], "...");
//         gphoto.openCam(id, list[id],
//         function(camera) {
//             console.log(camera);
//             gphoto.takePicture(camera,
//             function(result) {
//               console.log("Take picture returned:",result);
//             });
//         });
//     }
// });