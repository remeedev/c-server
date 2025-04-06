console.log("Hello, world!");

let link_list = [
    "/non-existent.css",
    "/non-supported.gif",
    "/favicon.ico",
    "/index.js",
    "/style.css"
]

for (let i = 0; i < link_list.length; i++){
    let link = link_list[i];
    console.log("Opening link "+i+": [" + link + "]");
    fetch(link);
}
