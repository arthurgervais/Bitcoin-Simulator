function openspvTab(evt, tabName) {
    // Declare all variables
    var i, innertabcontent, tablinks;

    // Get all elements with class="innertabcontent" and hide them
    innertabcontent = document.getElementsByClassName("spvTabContent");
    for (i = 0; i < innertabcontent.length; i++) {
        innertabcontent[i].style.display = "none";
    }

    // Get all elements with class="tablinks" and remove the class "active"
    tablinks = document.getElementsByClassName("spvTab");
    for (i = 0; i < tablinks.length; i++) {
        tablinks[i].className = tablinks[i].className.replace(" special", "");
    }

    // Show the current tab, and add an "active" class to the link that opened the tab
    document.getElementById(tabName).style.display = "block";
    evt.currentTarget.className += " special";
}