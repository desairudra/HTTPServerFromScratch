console.log("JavaScript Loaded!");

document.addEventListener("DOMContentLoaded", function () {

    const heading = document.querySelector("h1");

    heading.innerHTML = "Served by My C++ HTTP Server";

    heading.style.fontSize = "50px";

    const para = document.createElement("p");
    para.innerHTML = "JavaScript is being served by my own HTTP server.";

    document.body.appendChild(para);

});