package tn.essat.coagbackend.controller;

import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.GetMapping;

@Controller
public class HomeCRT {

    @GetMapping("/")
    public String home() {
        return "redirect:/index.html";
    }
}