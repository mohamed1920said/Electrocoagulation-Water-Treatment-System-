package tn.essat.coagbackend.controller;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;


import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ThreadLocalRandom;


@CrossOrigin("*")
@RestController
@RequestMapping("/api")
public class AppCRT {

    private static double latestPh = ThreadLocalRandom.current().nextDouble(6.6, 7.5);
    private static double latestTemp = ThreadLocalRandom.current().nextDouble(24.3, 26.5);
    private static double latestDistance = 0;
    private static boolean latestMixing = false;
    private static boolean latestFill = false;
    private static boolean latestDrain = false;
    private static boolean latestAcide = false;
    private static boolean latestBase = false;

    private static int auto_mode = 1;
    private static int pompe_fill_status = 0;
    private static int pompe_acide_status = 0;
    private static int pompe_drain_status = 0;
    private static int pompe_base_status = 0;
    private static int mixeur_status = 0;
    private static int relay_status = 1;

//ok
    @PostMapping("/data")
    public void receiveDistance(@RequestBody Map<String, Object> data) {
        latestDistance = ((Number) data.get("distance")).doubleValue();
        latestFill = (int) data.get("pompe_fill") == 1;
        latestDrain = (int) data.get("pompe_drain") == 1;
        latestMixing =  (int) data.get("mixeur") == 1 ;

        System.out.println("\n\n______________________________\nReceived from Arduino:" +
                "\ndistance " + latestDistance + "cm" +
                "\npompe_fill: " + latestFill +
                "\npompe_drain: " + latestDrain +
                "\nmixeur: " + latestMixing +
                "\npH: " + latestPh +
                "\ntemperature: " + latestTemp +
                "\npompe_base: " + latestBase +
                "\npompe_acide: " + latestAcide
                );
    }
//ok
@GetMapping("/stream")
public ResponseEntity<Map<String, Object>> streamToArduino() {
    // Variation légère entre -0.05 et +0.05
    double phChange = ThreadLocalRandom.current().nextDouble(-0.1, 0.1);
    double tempChange = ThreadLocalRandom.current().nextDouble(-0.1, 0.1);

    // Appliquer variation
    latestPh += phChange;
    latestTemp += tempChange;

    // Limiter pH entre 6.6 et 7.5
    if (latestPh < 6.6) latestPh = 6.6;
    if (latestPh > 7.5) latestPh = 7.5;

    // Limiter Température entre 24.3 et 26.5
    if (latestTemp < 24.3) latestTemp = 24.3;
    if (latestTemp > 26.5) latestTemp = 26.5;

    // Arrondir à 2 décimales
    latestPh = Math.round(latestPh * 100.0) / 100.0;
    latestTemp = Math.round(latestTemp * 100.0) / 100.0;

    Map<String, Object> response = new HashMap<>();
    response.put("ph", latestPh);
    response.put("temp", latestTemp);
    response.put("distance", latestDistance);
    response.put("pompe_fill", latestFill ? 1 : 0);
    response.put("pompe_drain", latestDrain ? 1 : 0);
    response.put("mixeur", latestMixing ? 1 : 0);
    response.put("pompe_acide", latestAcide ? 1 : 0);
    response.put("pompe_base", latestBase ? 1 : 0);
    return ResponseEntity.ok(response);
}


}
